/*  Copyright (C) 2014  Adam Green (https://github.com/adamgreen)

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/
#include <common.h>
#include <gdb_console.h>
#include <IMemory.h>
#include <signal.h>
#include <string.h>
#include <MemorySim.h>
#include <mri.h>
#include <mri4sim.h>
#include <NewlibSemihost.h>
#include <platforms.h>
#include <printfSpy.h>
#include <semihost.h>
#include "NewlibPriv.h"


/* NOTE: This is the original version of the following XML which has had things stripped to reduce the amount of
         FLASH consumed by the debug monitor.  This includes the removal of the copyright comment.
<?xml version="1.0"?>
<!-- Copyright (C) 2010, 2011 Free Software Foundation, Inc.

     Copying and distribution of this file, with or without modification,
     are permitted in any medium without royalty provided the copyright
     notice and this notice are preserved.  -->

<!DOCTYPE feature SYSTEM "gdb-target.dtd">
<feature name="org.gnu.gdb.arm.m-profile">
  <reg name="r0" bitsize="32"/>
  <reg name="r1" bitsize="32"/>
  <reg name="r2" bitsize="32"/>
  <reg name="r3" bitsize="32"/>
  <reg name="r4" bitsize="32"/>
  <reg name="r5" bitsize="32"/>
  <reg name="r6" bitsize="32"/>
  <reg name="r7" bitsize="32"/>
  <reg name="r8" bitsize="32"/>
  <reg name="r9" bitsize="32"/>
  <reg name="r10" bitsize="32"/>
  <reg name="r11" bitsize="32"/>
  <reg name="r12" bitsize="32"/>
  <reg name="sp" bitsize="32" type="data_ptr"/>
  <reg name="lr" bitsize="32"/>
  <reg name="pc" bitsize="32" type="code_ptr"/>
  <reg name="xpsr" bitsize="32" regnum="25"/>
</feature>
*/
static const char g_targetXML[] =
    "<?xml version=\"1.0\"?>\n"
    "<!DOCTYPE feature SYSTEM \"gdb-target.dtd\">\n"
    "<target>\n"
    "<feature name=\"org.gnu.gdb.arm.m-profile\">\n"
    "<reg name=\"r0\" bitsize=\"32\"/>\n"
    "<reg name=\"r1\" bitsize=\"32\"/>\n"
    "<reg name=\"r2\" bitsize=\"32\"/>\n"
    "<reg name=\"r3\" bitsize=\"32\"/>\n"
    "<reg name=\"r4\" bitsize=\"32\"/>\n"
    "<reg name=\"r5\" bitsize=\"32\"/>\n"
    "<reg name=\"r6\" bitsize=\"32\"/>\n"
    "<reg name=\"r7\" bitsize=\"32\"/>\n"
    "<reg name=\"r8\" bitsize=\"32\"/>\n"
    "<reg name=\"r9\" bitsize=\"32\"/>\n"
    "<reg name=\"r10\" bitsize=\"32\"/>\n"
    "<reg name=\"r11\" bitsize=\"32\"/>\n"
    "<reg name=\"r12\" bitsize=\"32\"/>\n"
    "<reg name=\"sp\" bitsize=\"32\" type=\"data_ptr\"/>\n"
    "<reg name=\"lr\" bitsize=\"32\"/>\n"
    "<reg name=\"pc\" bitsize=\"32\" type=\"code_ptr\"/>\n"
    "<reg name=\"xpsr\" bitsize=\"32\" regnum=\"25\"/>\n"
    "</feature>\n"
    "</target>\n";


static PinkySimContext g_context;
static IComm*          g_pComm;
static char            g_packetBuffer[16 * 1024];
static int             g_runResult;
static uint32_t        g_pcOrig;
static int             g_singleStepping;
static int             g_memoryFaultEncountered;


/* Core MRI function not exposed in public header since typically called by ASM. */
void __mriDebugException(void);

/* Forward static function declarations. */
static int shouldInterruptRun(PinkySimContext* pContext);
static int isExitSemihost(void);
static void logMessageToLocalAndGdbConsoles(const char* pMessage);
static int isInstruction32Bit(uint16_t firstWordOfInstruction);
static void sendRegisterForTResponse(Buffer* pBuffer, uint8_t registerOffset, uint32_t registerValue);
static void writeBytesToBufferAsHex(Buffer* pBuffer, void* pBytes, size_t byteCount);
static void readBytesFromBufferAsHex(Buffer* pBuffer, void* pBytes, size_t byteCount);
static uint32_t convertWatchpointTypeToMemorySimType(PlatformWatchpointType type);
static uint16_t getFirstHalfWordOfCurrentInstruction(void);
static int isInstructionNewlibSemihostBreakpoint(uint16_t instruction);
static int isInstructionHardcodedBreakpoint(uint16_t instruction);


__throws void mri4simInit(IMemory* pMem)
{
    memset(&g_context, 0x00, sizeof(g_context));
    g_context.spMain = IMemory_Read32(pMem, 0x00000000);
    g_context.pc = IMemory_Read32(pMem, 0x00000004) & 0xFFFFFFFE;
    g_context.xPSR |= EPSR_T;
    g_context.pMemory = pMem;
    g_singleStepping = 0;
    g_memoryFaultEncountered = 0;

    __mriInit("");
}


void mri4simRun(IComm* pComm, int breakOnStart)
{
    g_pComm = pComm;
    do
    {
        if (breakOnStart)
        {
            g_runResult = PINKYSIM_STEP_BKPT;
            breakOnStart = FALSE;
        }
        else
        {
            g_runResult = pinkySimRun(&g_context, shouldInterruptRun);
            if (isExitSemihost())
                break;
        }
        __mriDebugException();
    } while (!IComm_ShouldStopRun(pComm));
}

static int shouldInterruptRun(PinkySimContext* pContext)
{
    if (g_singleStepping > 1)
        g_singleStepping--;
    else if (g_singleStepping == 1)
        return PINKYSIM_RUN_SINGLESTEP;

    if (MemorySim_WasWatchpointEncountered(g_context.pMemory))
        return PINKYSIM_RUN_WATCHPOINT;

    if (IComm_IsGdbConnected(g_pComm) && IComm_HasReceiveData(g_pComm))
        return PINKYSIM_RUN_INTERRUPT;
    return PINKYSIM_STEP_OK;
}

static int isExitSemihost(void)
{
    static const uint16_t newlibExitBreakpointMachineCode = 0xbeff;
    uint16_t              currentInstruction;

    if (g_runResult != PINKYSIM_STEP_BKPT)
        return FALSE;

    __try
    {
        currentInstruction = getFirstHalfWordOfCurrentInstruction();
    }
    __catch
    {
        /* Will get here if PC isn't pointing to valid memory so treat as not exit request. */
        clearExceptionCode();
        return FALSE;
    }

    return currentInstruction == newlibExitBreakpointMachineCode;
}

PinkySimContext* mri4simGetContext(void)
{
    return &g_context;
}



void Platform_Init(Token* pParameterTokens)
{
}

char* Platform_GetPacketBuffer(void)
{
    return g_packetBuffer;
}

uint32_t  Platform_GetPacketBufferSize(void)
{
    return sizeof(g_packetBuffer);
}

void Platform_EnteringDebugger(void)
{
    g_pcOrig = g_context.pc;
    Platform_DisableSingleStep();
}

void Platform_LeavingDebugger(void)
{
}

uint32_t Platform_MemRead32(const void* pv)
{
    uint32_t retVal = 0;
    __try
        retVal = IMemory_Read32(g_context.pMemory, (uint32_t)pv);
    __catch
        g_memoryFaultEncountered++;
    return retVal;
}

uint16_t Platform_MemRead16(const void* pv)
{
    uint16_t retVal = 0;
    __try
        retVal = IMemory_Read16(g_context.pMemory, (uint32_t)pv);
    __catch
        g_memoryFaultEncountered++;
    return retVal;
}

uint8_t Platform_MemRead8(const void* pv)
{
    uint8_t retVal = 0;
    __try
        retVal = IMemory_Read8(g_context.pMemory, (uint32_t)pv);
    __catch
        g_memoryFaultEncountered++;
    return retVal;
}

void Platform_MemWrite32(void* pv, uint32_t value)
{
    __try
        IMemory_Write32(g_context.pMemory, (uint32_t)pv, value);
    __catch
        g_memoryFaultEncountered++;
}

void Platform_MemWrite16(void* pv, uint16_t value)
{
    __try
        IMemory_Write16(g_context.pMemory, (uint32_t)pv, value);
    __catch
        g_memoryFaultEncountered++;
}

void Platform_MemWrite8(void* pv, uint8_t value)
{
    __try
        IMemory_Write8(g_context.pMemory, (uint32_t)pv, value);
    __catch
        g_memoryFaultEncountered++;
}

uint32_t Platform_CommHasReceiveData(void)
{
    return IComm_HasReceiveData(g_pComm);
}

int Platform_CommReceiveChar(void)
{
    return IComm_ReceiveChar(g_pComm);
}

void Platform_CommSendChar(int character)
{
    IComm_SendChar(g_pComm, character);
}

int Platform_CommCausedInterrupt(void)
{
    return FALSE;
}

void Platform_CommClearInterrupt(void)
{
}

int Platform_CommShouldWaitForGdbConnect(void)
{
    return FALSE;
}

int Platform_CommSharingWithApplication(void)
{
    return FALSE;
}

void Platform_CommPrepareToWaitForGdbConnection(void)
{
}

int Platform_CommIsWaitingForGdbToConnect(void)
{
    return !IComm_IsGdbConnected(g_pComm);
}

void Platform_CommWaitForReceiveDataToStop(void)
{
}


uint8_t Platform_DetermineCauseOfException(void)
{
    uint8_t signal = SIGSTOP;

    switch (g_runResult)
    {
    case PINKYSIM_STEP_UNDEFINED:
    case PINKYSIM_STEP_UNPREDICTABLE:
    case PINKYSIM_STEP_UNSUPPORTED:
    case PINKYSIM_STEP_SVC:
        signal = SIGILL;
        break;
    case PINKYSIM_STEP_HARDFAULT:
        signal = SIGSEGV;
        break;
    case PINKYSIM_STEP_BKPT:
    case PINKYSIM_RUN_WATCHPOINT:
    case PINKYSIM_RUN_SINGLESTEP:
        signal = SIGTRAP;
        break;
    case PINKYSIM_RUN_INTERRUPT:
        signal = SIGINT;
        break;
    }

    return signal;
}

void Platform_DisplayFaultCauseToGdbConsole(void)
{
    switch (g_runResult)
    {
    case PINKYSIM_STEP_UNDEFINED:
        logMessageToLocalAndGdbConsoles("\n**Undefined Instruction**\n");
        break;
    case PINKYSIM_STEP_UNPREDICTABLE:
        logMessageToLocalAndGdbConsoles("\n**Unpredictable Instruction Encoding**\n");
        break;
    case PINKYSIM_STEP_UNSUPPORTED:
    case PINKYSIM_STEP_SVC:
        logMessageToLocalAndGdbConsoles("\n**Unsupported Instruction**\n");
        break;
    case PINKYSIM_STEP_HARDFAULT:
        logMessageToLocalAndGdbConsoles("\n**Hard Fault**\n");
        break;
    }
}

static void logMessageToLocalAndGdbConsoles(const char* pMessage)
{
    printf("%s", pMessage);
    WriteStringToGdbConsole(pMessage);
}


void Platform_EnableSingleStep(void)
{
    g_singleStepping = 2;
}

void Platform_DisableSingleStep(void)
{
    g_singleStepping = 0;
}

int Platform_IsSingleStepping(void)
{
    return g_singleStepping > 0;
}

void Platform_SetProgramCounter(uint32_t newPC)
{
    g_context.pc = newPC;
}

void Platform_AdvanceProgramCounterToNextInstruction(void)
{
    uint16_t  firstWordOfCurrentInstruction;

    __try
    {
        firstWordOfCurrentInstruction = getFirstHalfWordOfCurrentInstruction();
    }
    __catch
    {
        /* Will get here if PC isn't pointing to valid memory so don't bother to advance. */
        clearExceptionCode();
        return;
    }

    if (isInstruction32Bit(firstWordOfCurrentInstruction))
    {
        /* 32-bit Instruction. */
        g_context.pc += 4;
    }
    else
    {
        /* 16-bit Instruction. */
        g_context.pc += 2;
    }
}

static int isInstruction32Bit(uint16_t firstWordOfInstruction)
{
    uint16_t maskedOffUpper5BitsOfWord = firstWordOfInstruction & 0xF800;

    /* 32-bit instructions start with 0b11101, 0b11110, 0b11111 according to page A5-152 of the
       ARMv7-M Architecture Manual. */
    return  (maskedOffUpper5BitsOfWord == 0xE800 ||
             maskedOffUpper5BitsOfWord == 0xF000 ||
             maskedOffUpper5BitsOfWord == 0xF800);
}

int Platform_WasProgramCounterModifiedByUser(void)
{
    return g_context.pc != g_pcOrig;
}

int Platform_WasMemoryFaultEncountered(void)
{
    int memoryFaultEncountered = g_memoryFaultEncountered;
    g_memoryFaultEncountered = 0;
    return memoryFaultEncountered;
}


void Platform_WriteTResponseRegistersToBuffer(Buffer* pBuffer)
{
    sendRegisterForTResponse(pBuffer, 12, g_context.R[12]);
    sendRegisterForTResponse(pBuffer, 13, g_context.spMain);
    sendRegisterForTResponse(pBuffer, 14, g_context.lr);
    sendRegisterForTResponse(pBuffer, 15, g_context.pc);
}

static void sendRegisterForTResponse(Buffer* pBuffer, uint8_t registerOffset, uint32_t registerValue)
{
    Buffer_WriteByteAsHex(pBuffer, registerOffset);
    Buffer_WriteChar(pBuffer, ':');
    writeBytesToBufferAsHex(pBuffer, &registerValue, sizeof(registerValue));
    Buffer_WriteChar(pBuffer, ';');
}

static void writeBytesToBufferAsHex(Buffer* pBuffer, void* pBytes, size_t byteCount)
{
    uint8_t* pByte = (uint8_t*)pBytes;
    size_t   i;

    for (i = 0 ; i < byteCount ; i++)
        Buffer_WriteByteAsHex(pBuffer, *pByte++);
}


void Platform_CopyContextToBuffer(Buffer* pBuffer)
{
    writeBytesToBufferAsHex(pBuffer, &g_context.R[0], (16 + 1) * sizeof(uint32_t));
}


void Platform_CopyContextFromBuffer(Buffer* pBuffer)
{
    readBytesFromBufferAsHex(pBuffer, &g_context.R[0], (16 + 1) * sizeof(uint32_t));
}

static void readBytesFromBufferAsHex(Buffer* pBuffer, void* pBytes, size_t byteCount)
{
    uint8_t* pByte = (uint8_t*)pBytes;
    size_t   i;

    for (i = 0 ; i < byteCount; i++)
        *pByte++ = Buffer_ReadByteAsHex(pBuffer);
}


uint32_t Platform_GetDeviceMemoryMapXmlSize(void)
{
    return strlen(MemorySim_GetMemoryMapXML(g_context.pMemory));
}

const char* Platform_GetDeviceMemoryMapXml(void)
{
    return MemorySim_GetMemoryMapXML(g_context.pMemory);
}


uint32_t Platform_GetTargetXmlSize(void)
{
    return sizeof(g_targetXML) - 1;
}


const char* Platform_GetTargetXml(void)
{
    return g_targetXML;
}


__throws void Platform_SetHardwareBreakpoint(uint32_t address, uint32_t kind)
{
    uint32_t size;

    switch (kind)
    {
    case 2:
        size = 2;
        break;
    case 3:
    case 4:
        size = 4;
        break;
    default:
        __mriExceptionCode = invalidArgumentException;
        return;
    }

    __try
        MemorySim_SetHardwareBreakpoint(g_context.pMemory, address, size);
    __catch
        __mriExceptionCode = exceededHardwareResourcesException;
}


__throws void Platform_ClearHardwareBreakpoint(uint32_t address, uint32_t kind)
{
    uint32_t size;

    switch (kind)
    {
    case 2:
        size = 2;
        break;
    case 3:
    case 4:
        size = 4;
        break;
    default:
        __mriExceptionCode = invalidArgumentException;
        return;
    }

    __try
        MemorySim_ClearHardwareBreakpoint(g_context.pMemory, address, size);
    __catch
        __mriExceptionCode = invalidArgumentException;
}


__throws void Platform_SetHardwareWatchpoint(uint32_t address, uint32_t size, PlatformWatchpointType type)
{
    __try
        MemorySim_SetHardwareWatchpoint(g_context.pMemory, address, size, convertWatchpointTypeToMemorySimType(type));
    __catch
        __mriExceptionCode = exceededHardwareResourcesException;
}

static WatchpointType convertWatchpointTypeToMemorySimType(PlatformWatchpointType type)
{
    WatchpointType simType = 0;

    switch (type)
    {
    case MRI_PLATFORM_WRITE_WATCHPOINT:
        simType = WATCHPOINT_WRITE;
        break;
    case MRI_PLATFORM_READ_WATCHPOINT:
        simType = WATCHPOINT_READ;
        break;
    case MRI_PLATFORM_READWRITE_WATCHPOINT:
        simType = WATCHPOINT_READ_WRITE;
        break;
    }
    return simType;
}


__throws void Platform_ClearHardwareWatchpoint(uint32_t address, uint32_t size,  PlatformWatchpointType type)
{
    __try
        MemorySim_ClearHardwareWatchpoint(g_context.pMemory, address, size, convertWatchpointTypeToMemorySimType(type));
    __catch
        __mriExceptionCode = invalidArgumentException;
}


PlatformInstructionType Platform_TypeOfCurrentInstruction(void)
{
    uint16_t currentInstruction;

    __try
    {
        currentInstruction = getFirstHalfWordOfCurrentInstruction();
    }
    __catch
    {
        /* Will get here if PC isn't pointing to valid memory so treat as other. */
        clearExceptionCode();
        return MRI_PLATFORM_INSTRUCTION_OTHER;
    }

    if (isInstructionNewlibSemihostBreakpoint(currentInstruction))
        return MRI_PLATFORM_INSTRUCTION_NEWLIB_SEMIHOST_CALL;
    else if (isInstructionHardcodedBreakpoint(currentInstruction))
        return MRI_PLATFORM_INSTRUCTION_HARDCODED_BREAKPOINT;
    else
        return MRI_PLATFORM_INSTRUCTION_OTHER;
}

static uint16_t getFirstHalfWordOfCurrentInstruction(void)
{
    return IMemory_Read16(g_context.pMemory, g_context.pc);
}

static int isInstructionNewlibSemihostBreakpoint(uint16_t instruction)
{
    static const uint16_t breakpointOpcode = 0xbe00;
    static const uint16_t opcodeMask       = 0xff00;
    static const uint16_t immediateMask    = 0x00ff;

    if ((instruction & opcodeMask) == breakpointOpcode)
    {
        uint16_t immediate = instruction & immediateMask;

        return immediate >= NEWLIB_MIN && immediate <= NEWLIB_MAX;
    }
    return FALSE;
}

static int isInstructionHardcodedBreakpoint(uint16_t instruction)
{
    static const uint16_t hardCodedBreakpointMachineCode = 0xbe00;

    return (hardCodedBreakpointMachineCode == instruction);
}


PlatformSemihostParameters Platform_GetSemihostCallParameters(void)
{
    PlatformSemihostParameters parameters;

    parameters.parameter1 = g_context.R[0];
    parameters.parameter2 = g_context.R[1];
    parameters.parameter3 = g_context.R[2];
    parameters.parameter4 = g_context.R[3];

    return parameters;
}


void Platform_SetSemihostCallReturnAndErrnoValues(int returnValue, int err)
{
    g_context.R[0] = returnValue;
    g_context.R[1] = err;
}


int Semihost_IsDebuggeeMakingSemihostCall(void)
{
    PlatformInstructionType instructionType = Platform_TypeOfCurrentInstruction();

    return (instructionType == MRI_PLATFORM_INSTRUCTION_NEWLIB_SEMIHOST_CALL);
}


int Semihost_HandleSemihostRequest(void)
{
    static const uint16_t      immediateMask    = 0x00ff;
    PlatformSemihostParameters parameters = Platform_GetSemihostCallParameters();
    int                        result = 0;

    switch (getFirstHalfWordOfCurrentInstruction() & immediateMask)
    {
    case NEWLIB_WRITE:
        result = handleNewlibSemihostWriteRequest(&parameters);
        break;
    case NEWLIB_READ:
        result = handleNewlibSemihostReadRequest(&parameters);
        break;
    case NEWLIB_OPEN:
        result = handleNewlibSemihostOpenRequest(&parameters);
        break;
    case NEWLIB_UNLINK:
        result = handleNewlibSemihostUnlinkRequest(&parameters);
        break;
    case NEWLIB_LSEEK:
        result = handleNewlibSemihostLSeekRequest(&parameters);
        break;
    case NEWLIB_CLOSE:
        result = handleNewlibSemihostCloseRequest(&parameters);
        break;
    case NEWLIB_FSTAT:
        result = handleNewlibSemihostFStatRequest(&parameters);
        break;
    case NEWLIB_STAT:
        result = handleNewlibSemihostStatRequest(&parameters);
        break;
    case NEWLIB_RENAME:
        result = handleNewlibSemihostRenameRequest(&parameters);
        break;
    }
    return result;
}


void __mriPlatform_EnteringDebuggerHook(void)
{
}

void __mriPlatform_LeavingDebuggerHook(void)
{
}

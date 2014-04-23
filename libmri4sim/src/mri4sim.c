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
#include <string.h>
#include <mri.h>
#include <mri4sim.h>
#include <platforms.h>
#include <semihost.h>


#define FALSE 0
#define TRUE  1

static PinkySimContext g_context;


__throws void mri4simInit(IMemory* pMem)
{
    memset(&g_context, 0x00, sizeof(g_context));
    g_context.spMain = IMemory_Read32(pMem, 0x00000000);
    g_context.pc = IMemory_Read32(pMem, 0x00000004) & 0xFFFFFFFE;
    g_context.xPSR |= EPSR_T;
    g_context.pMemory = pMem;
    
    __mriInit("");
}


void mri4simRun(IComm* pComm)
{
    do
    {
        if (IComm_ShouldExit(pComm))
            break;
    } while (1);
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
    return NULL;
}

uint32_t  Platform_GetPacketBufferSize(void)
{
    return 0;
}

void Platform_EnteringDebugger(void)
{
}

void Platform_LeavingDebugger(void)
{
}

uint32_t Platform_MemRead32(const void* pv)
{
    return 0;
}

uint16_t Platform_MemRead16(const void* pv)
{
    return 0;
}

uint8_t Platform_MemRead8(const void* pv)
{
    return 0;
}

void Platform_MemWrite32(void* pv, uint32_t value)
{
}

void Platform_MemWrite16(void* pv, uint16_t value)
{
}

void Platform_MemWrite8(void* pv, uint8_t value)
{
}

uint32_t Platform_CommHasReceiveData(void)
{
    return FALSE;
}

int Platform_CommReceiveChar(void)
{
    return 0;
}

void Platform_CommSendChar(int character)
{
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
    return FALSE;
}

void Platform_CommWaitForReceiveDataToStop(void)
{
}

int Platform_CommUartIndex(void)
{
    return 0;
}

uint8_t Platform_DetermineCauseOfException(void)
{
    return 0;
}

void Platform_DisplayFaultCauseToGdbConsole(void)
{
}

void Platform_EnableSingleStep(void)
{
}

void Platform_DisableSingleStep(void)
{
}

int Platform_IsSingleStepping(void)
{
    return FALSE;
}

uint32_t Platform_GetProgramCounter(void)
{
    return 0;
}

void Platform_SetProgramCounter(uint32_t newPC)
{
}

void Platform_AdvanceProgramCounterToNextInstruction(void)
{
}

int Platform_WasProgramCounterModifiedByUser(void)
{
    return FALSE;
}

int Platform_WasMemoryFaultEncountered(void)
{
    return FALSE;
}

void Platform_WriteTResponseRegistersToBuffer(Buffer* pBuffer)
{
}

void Platform_CopyContextToBuffer(Buffer* pBuffer)
{
}

void Platform_CopyContextFromBuffer(Buffer* pBuffer)
{
}

uint32_t Platform_GetDeviceMemoryMapXmlSize(void)
{
    return 0;
}

const char* Platform_GetDeviceMemoryMapXml(void)
{
    return NULL;
}

uint32_t Platform_GetTargetXmlSize(void)
{
    return 0;
}

const char* Platform_GetTargetXml(void)
{
    return NULL;
}

__throws void Platform_SetHardwareBreakpoint(uint32_t address, uint32_t kind)
{
}

__throws void Platform_ClearHardwareBreakpoint(uint32_t address, uint32_t kind)
{
}

__throws void Platform_SetHardwareWatchpoint(uint32_t address, uint32_t size,  PlatformWatchpointType type)
{
}

__throws void Platform_ClearHardwareWatchpoint(uint32_t address, uint32_t size,  PlatformWatchpointType type)
{
}

PlatformInstructionType Platform_TypeOfCurrentInstruction(void)
{
    return MRI_PLATFORM_INSTRUCTION_OTHER;
}

PlatformSemihostParameters Platform_GetSemihostCallParameters(void)
{
    PlatformSemihostParameters params;
    
    memset(&params, 0, sizeof(params));
    return params;
}

void Platform_SetSemihostCallReturnValue(uint32_t returnValue)
{
}

int Semihost_IsDebuggeeMakingSemihostCall(void)
{
    return FALSE;
}

int Semihost_HandleSemihostRequest(void)
{
    return FALSE;
}

void __mriPlatform_EnteringDebuggerHook(void)
{
}

void __mriPlatform_LeavingDebuggerHook(void)
{
}


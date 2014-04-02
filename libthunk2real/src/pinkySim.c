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
#include <gdbRemote.h>
#include <pinkySim.h>


static int determineHardFaultCause(PinkySimContext* pContext);
static int wasSVC(PinkySimContext* pContext);
static int wasUndefinedInstruction(PinkySimContext* pContext);
static int wasBKPT(PinkySimContext* pContext);


int pinkySimStep(PinkySimContext* pContext)
{
    int signal;
    
    gdbRemoteSetRegisters(pContext);
    signal = gdbRemoteSingleStep();
    gdbRemoteGetRegisters(pContext);
    
    switch (signal)
    {
    case SIGSEGV:
        return determineHardFaultCause(pContext);
    case SIGTRAP:
        if (wasBKPT(pContext))
            return PINKYSIM_STEP_BKPT;
        else
            return PINKYSIM_STEP_OK;
    default:
        return PINKYSIM_STEP_OK;
    }
}

static int determineHardFaultCause(PinkySimContext* pContext)
{
    if (wasSVC(pContext))
        return PINKYSIM_STEP_SVC;
    if (wasUndefinedInstruction(pContext))
        return PINKYSIM_STEP_UNDEFINED;
    return PINKYSIM_STEP_HARDFAULT;
}

static int wasSVC(PinkySimContext* pContext)
{
    uint32_t pc = pContext->pc - 2;
    uint16_t instr;
    
    if (pc < 0x10000000 || pc >= 0x10008000)
        return 0;
        
    instr = IMemory_Read16(pContext->pMemory, pc);
    if ((instr & 0xFF00) == 0xDF00)
        // SVC call
        return 1;
    else
        return 0;
}

static int wasUndefinedInstruction(PinkySimContext* pContext)
{
    static const uint16_t undefinstrBit = (1 << 0);
    uint16_t              usageFaultStatusRegister;
    
    gdbRemoteReadMemory(0xE000ED2A, &usageFaultStatusRegister, sizeof(usageFaultStatusRegister));
    if (usageFaultStatusRegister & undefinstrBit)
        return 1;
    return 0;
}

static int wasBKPT(PinkySimContext* pContext)
{
    uint32_t pc = pContext->pc;
    uint16_t instr;
    
    if (pc < 0x10000000 || pc >= 0x10008000)
        return 0;
        
    instr = IMemory_Read16(pContext->pMemory, pc);
    if ((instr & 0xFF00) == 0xBE00)
        // BKPT call
        return 1;
    else
        return 0;
}

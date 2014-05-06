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

#include "pinkySimBaseTest.h"

TEST_GROUP_BASE(pop, pinkySimBase)
{
    void setup()
    {
        pinkySimBase::setup();
    }

    void teardown()
    {
        pinkySimBase::teardown();
    }
};


/* POP
   Encoding: 1011 1 10 P:1 RegisterList:8 */
TEST(pop, JustPopPC)
{
    emitInstruction16("1011110Prrrrrrrr", 1, 0);
    setRegisterValue(SP, INITIAL_SP - 4);
    setExpectedRegisterValue(SP, INITIAL_SP);
    setExpectedRegisterValue(PC, INITIAL_PC + 16);
    SimpleMemory_SetMemory(m_context.pMemory, INITIAL_SP - 4, (INITIAL_PC + 16) | 1, READ_ONLY);
    pinkySimStep(&m_context);
}

TEST(pop, JustPopR0)
{
    emitInstruction16("1011110Prrrrrrrr", 0, 1);
    setRegisterValue(SP, INITIAL_SP - 4);
    setExpectedRegisterValue(SP, INITIAL_SP);
    setExpectedRegisterValue(R0, 0xFFFFFFFF);
    SimpleMemory_SetMemory(m_context.pMemory, INITIAL_SP - 4, 0xFFFFFFFF, READ_ONLY);
    pinkySimStep(&m_context);
}

TEST(pop, JustPopR7)
{
    emitInstruction16("1011110Prrrrrrrr", 0, (1 << 7));
    setRegisterValue(SP, INITIAL_SP - 4);
    setExpectedRegisterValue(SP, INITIAL_SP);
    setExpectedRegisterValue(R7, 0xFFFFFFFF);
    SimpleMemory_SetMemory(m_context.pMemory, INITIAL_SP - 4, 0xFFFFFFFF, READ_ONLY);
    pinkySimStep(&m_context);
}

TEST(pop, PopAll)
{
    emitInstruction16("1011110Prrrrrrrr", 1, 0xFF);
    setRegisterValue(SP, INITIAL_SP - 4 * 9);
    setExpectedRegisterValue(SP, INITIAL_SP);
    setExpectedRegisterValue(R0, 9);
    setExpectedRegisterValue(R1, 8);
    setExpectedRegisterValue(R2, 7);
    setExpectedRegisterValue(R3, 6);
    setExpectedRegisterValue(R4, 5);
    setExpectedRegisterValue(R5, 4);
    setExpectedRegisterValue(R6, 3);
    setExpectedRegisterValue(R7, 2);
    setExpectedRegisterValue(PC, 1 & ~1);
    for (int i = 1 ; i <= 9 ; i++)
        SimpleMemory_SetMemory(m_context.pMemory, INITIAL_SP - 4 * i, i, READ_ONLY);
    pinkySimStep(&m_context);
}

TEST(pop, PopToSetPCToEvenAddressWhichGeneratesHardFault)
{
    emitInstruction16("1011110Prrrrrrrr", 1, 0);
    setExpectedXPSRflags("t");
    setRegisterValue(SP, INITIAL_SP - 4);
    setExpectedRegisterValue(SP, INITIAL_SP);
    setExpectedRegisterValue(PC, INITIAL_PC + 16);
    SimpleMemory_SetMemory(m_context.pMemory, INITIAL_SP - 4, INITIAL_PC + 16, READ_ONLY);
    pinkySimStep(&m_context);
    
    const uint16_t NOP = 0xBF00;
    SimpleMemory_SetMemory(m_context.pMemory, INITIAL_PC + 16, NOP, READ_ONLY);
    setExpectedStepReturn(PINKYSIM_STEP_HARDFAULT);
    pinkySimStep(&m_context);
}

TEST_SIM_ONLY(pop, HardFaultFromInvalidMemoryRead)
{
    emitInstruction16("1011110Prrrrrrrr", 0, 1);
    setRegisterValue(SP, 0xFFFFFFFC);
    setExpectedStepReturn(PINKYSIM_STEP_HARDFAULT);
    setExpectedRegisterValue(PC, INITIAL_PC);
    pinkySimStep(&m_context);
}

TEST_SIM_ONLY(pop, UnpredictableToPopNoRegisters)
{
    emitInstruction16("1011110Prrrrrrrr", 0, 0);
    setExpectedStepReturn(PINKYSIM_STEP_UNPREDICTABLE);
    setExpectedRegisterValue(PC, INITIAL_PC);
    pinkySimStep(&m_context);
}

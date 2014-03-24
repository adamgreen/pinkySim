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

TEST_GROUP_BASE(ldrImmediate, pinkySimBase)
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


/* LDR - Immediate Encoding T1
   Encoding: 011 0 1 Imm:5 Rn:3 Rt:3 */
TEST(ldrImmediate, T1UseAMixOfRegistersWithSmallestOffset)
{
    emitInstruction16("01101iiiiinnnttt", 0, R7, R0);
    setRegisterValue(R7, INITIAL_PC + 4);
    SimpleMemory_SetMemory(m_context.pMemory, INITIAL_PC + 4, 0xBAADFEED, READ_ONLY);
    setExpectedRegisterValue(R0, 0xBAADFEED);
    pinkySimStep(&m_context);
}

TEST(ldrImmediate, T1UseAnotherMixOfRegistersWithLargestOffset)
{
    emitInstruction16("01101iiiiinnnttt", 31, R0, R7);
    setRegisterValue(R0, INITIAL_PC);
    SimpleMemory_SetMemory(m_context.pMemory, INITIAL_PC + 31 * 4, 0xBAADFEED, READ_ONLY);
    setExpectedRegisterValue(R7, 0xBAADFEED);
    pinkySimStep(&m_context);
}

TEST(ldrImmediate, T1AttemptUnalignedLoad)
{
    emitInstruction16("01101iiiiinnnttt", 0, R3, R2);
    setRegisterValue(R3, INITIAL_PC + 2);
    setExpectedStepReturn(PINKYSIM_STEP_HARDFAULT);
    setExpectedRegisterValue(PC, INITIAL_PC);
    pinkySimStep(&m_context);
}

TEST(ldrImmediate, T1AttemptLoadFromInvalidAddress)
{
    emitInstruction16("01101iiiiinnnttt", 16, R3, R2);
    setRegisterValue(R3, 0xFFFFFFFC - 16 * 4);
    setExpectedStepReturn(PINKYSIM_STEP_HARDFAULT);
    setExpectedRegisterValue(PC, INITIAL_PC);
    pinkySimStep(&m_context);
}



/* LDR - Immediate Encoding T2 (SP is base register)
   Encoding: 1001 1 Rt:3 Imm:8 */
TEST(ldrImmediate, T2UseHighestRegisterWithSmallestOffset)
{
    emitInstruction16("10011tttiiiiiiii", R7, 0);
    setRegisterValue(SP, INITIAL_PC + 4);
    SimpleMemory_SetMemory(m_context.pMemory, INITIAL_PC + 4, 0xBAADFEED, READ_ONLY);
    setExpectedRegisterValue(R7, 0xBAADFEED);
    pinkySimStep(&m_context);
}

TEST(ldrImmediate, T2UseLowestRegisterWithLargestOffset)
{
    emitInstruction16("10011tttiiiiiiii", R0, 255);
    setRegisterValue(SP, INITIAL_PC);
    SimpleMemory_SetMemory(m_context.pMemory, INITIAL_PC + 255 * 4, 0xBAADFEED, READ_ONLY);
    setExpectedRegisterValue(R0, 0xBAADFEED);
    pinkySimStep(&m_context);
}

TEST(ldrImmediate, T2AttemptUnalignedLoad)
{
    emitInstruction16("10011tttiiiiiiii", R2, 0);
    setRegisterValue(SP, INITIAL_PC + 2);
    setExpectedStepReturn(PINKYSIM_STEP_HARDFAULT);
    setExpectedRegisterValue(PC, INITIAL_PC);
    pinkySimStep(&m_context);
}

TEST(ldrImmediate, T2AttemptLoadFromInvalidAddress)
{
    emitInstruction16("10011tttiiiiiiii", R2, 0);
    setRegisterValue(R3, 0xFFFFFFFC);
    setExpectedStepReturn(PINKYSIM_STEP_HARDFAULT);
    setExpectedRegisterValue(PC, INITIAL_PC);
    pinkySimStep(&m_context);
}

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

TEST_GROUP_BASE(ldrLiteral, pinkySimBase)
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


/* LDR - Literal
   Encoding: 01001 Rt:3 Imm:8 */
TEST(ldrLiteral, LoadOffset0IntoHighestRegister)
{
    emitInstruction16("01001tttiiiiiiii", R7, 0);
    SimpleMemory_SetMemory(m_context.pMemory, INITIAL_PC + 4, 0xBAADFEED, READ_ONLY);
    setExpectedRegisterValue(R7, 0xBAADFEED);
    pinkySimStep(&m_context);
}

TEST(ldrLiteral, LoadOffset0IntoHighestRegisterNot4ByteAligned)
{
    // Emit UNDEFINED 16-bit instruction.
    emitInstruction16("1101111000000000");
    // Emit actual test instruction at a 2-byte aligned address which isn't 4-byte aligned.
    emitInstruction16("01001tttiiiiiiii", R7, 0);
    SimpleMemory_SetMemory(m_context.pMemory, INITIAL_PC + 4, 0xBAADFEED, READ_ONLY);
    // Move PC to point to second instruction.
    setRegisterValue(PC, m_context.pc + 2);
    setExpectedRegisterValue(R7, 0xBAADFEED);
    pinkySimStep(&m_context);
}

TEST(ldrLiteral, LoadMaximumOffsetIntoLowestRegister)
{
    emitInstruction16("01001tttiiiiiiii", R0, 255);
    SimpleMemory_SetMemory(m_context.pMemory, INITIAL_PC + 4 + 255*4, 0xBAADFEED, READ_ONLY);
    setExpectedRegisterValue(R0, 0xBAADFEED);
    pinkySimStep(&m_context);
}

TEST(ldrLiteral, AttemptToLoadFromInvalidAddress)
{
    m_emitAddress = INITIAL_SP - 4;
    setRegisterValue(PC, INITIAL_SP - 4);
    setExpectedRegisterValue(PC, INITIAL_SP - 4);
    SimpleMemory_SetMemory(m_context.pMemory, INITIAL_SP - 4, 0, READ_WRITE);
    emitInstruction16("01001tttiiiiiiii", R0, 255);
    setExpectedStepReturn(PINKYSIM_STEP_HARDFAULT);
    pinkySimStep(&m_context);
}
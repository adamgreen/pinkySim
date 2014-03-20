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

TEST_GROUP_BASE(strhImmediate, pinkySimBase)
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


/* STRH - Immediate
   Encoding: 1000 0 Imm:5 Rn:3 Rt:3 */
TEST(strhImmediate, UseAMixOfRegistersWordAlignedWithSmallestOffset)
{
    emitInstruction16("10000iiiiinnnttt", 0, R7, R0);
    setRegisterValue(R7, INITIAL_PC + 4);
    SimpleMemory_SetMemory(m_context.pMemory, INITIAL_PC + 4, 0xBAADFEED, READ_WRITE);
    pinkySimStep(&m_context);
    CHECK_EQUAL(0xBAAD0000, IMemory_Read32(m_context.pMemory, INITIAL_PC + 4));
}

TEST(strhImmediate, AnotherMixOfRegistersNotWordAligned)
{
    emitInstruction16("10000iiiiinnnttt", 1, R0, R7);
    setRegisterValue(R0, INITIAL_PC + 4);
    SimpleMemory_SetMemory(m_context.pMemory, INITIAL_PC + 4, 0xBAADFEED, READ_WRITE);
    pinkySimStep(&m_context);
    CHECK_EQUAL(0x7777FEED, IMemory_Read32(m_context.pMemory, INITIAL_PC + 4));
}

TEST(strhImmediate, LargestOffset)
{
    emitInstruction16("10000iiiiinnnttt", 31, R0, R7);
    setRegisterValue(R0, INITIAL_PC);
    SimpleMemory_SetMemory(m_context.pMemory, INITIAL_PC + 60, 0xBAADFEED, READ_WRITE);
    pinkySimStep(&m_context);
    CHECK_EQUAL(0x7777FEED, IMemory_Read32(m_context.pMemory, INITIAL_PC + 60));
}
TEST(strhImmediate, AttemptStoreToInvalidAddress)
{
    emitInstruction16("10000iiiiinnnttt", 0, R3, R1);
    setRegisterValue(R3, 0xFFFFFFFC);
    setExpectedStepReturn(PINKYSIM_STEP_HARDFAULT);
    setExpectedRegisterValue(PC, INITIAL_PC);
    pinkySimStep(&m_context);
}

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

TEST_GROUP_BASE(strbImmediate, pinkySimBase)
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


/* STRB - Immediate
   Encoding: 011 1 0 Imm:5 Rn:3 Rt:3 */
TEST(strbImmediate, UseAMixOfRegistersWordAlignedWithSmallestOffset)
{
    emitInstruction16("01110iiiiinnnttt", 0, R7, R0);
    setRegisterValue(R7, INITIAL_PC + 4);
    SimpleMemory_SetMemory(m_context.pMemory, INITIAL_PC + 4, 0xBAADFEED, READ_WRITE);
    pinkySimStep(&m_context);
    CHECK_EQUAL(0xBAADFE00, IMemory_Read32(m_context.pMemory, INITIAL_PC + 4));
}

TEST(strbImmediate, UseAnotherMixOfRegistersSecondByteInWord)
{
    emitInstruction16("01110iiiiinnnttt", 1, R0, R7);
    setRegisterValue(R0, INITIAL_PC + 4);
    SimpleMemory_SetMemory(m_context.pMemory, INITIAL_PC + 4, 0xBAADFEED, READ_WRITE);
    pinkySimStep(&m_context);
    CHECK_EQUAL(0xBAAD77ED, IMemory_Read32(m_context.pMemory, INITIAL_PC + 4));
}

TEST(strbImmediate, YetAnotherMixOfRegistersThirdByteInWord)
{
    emitInstruction16("01110iiiiinnnttt", 2, R3, R4);
    setRegisterValue(R3, INITIAL_PC + 4);
    SimpleMemory_SetMemory(m_context.pMemory, INITIAL_PC + 4, 0xBAADFEED, READ_WRITE);
    pinkySimStep(&m_context);
    CHECK_EQUAL(0xBA44FEED, IMemory_Read32(m_context.pMemory, INITIAL_PC + 4));
}

TEST(strbImmediate, YetAnotherMixOfRegistersFourthByteInWord)
{
    emitInstruction16("01110iiiiinnnttt", 3, R1, R5);
    setRegisterValue(R1, INITIAL_PC + 4);
    SimpleMemory_SetMemory(m_context.pMemory, INITIAL_PC + 4, 0xBAADFEED, READ_WRITE);
    pinkySimStep(&m_context);
    CHECK_EQUAL(0x55ADFEED, IMemory_Read32(m_context.pMemory, INITIAL_PC + 4));
}

TEST(strbImmediate, AttemptStoreToInvalidAddressWithLargestOffset)
{
    emitInstruction16("01110iiiiinnnttt", 31, R3, R0);
    setRegisterValue(R3, 0xFFFFFFFC - 31);
    setExpectedStepReturn(PINKYSIM_STEP_HARDFAULT);
    setExpectedRegisterValue(PC, INITIAL_PC);
    pinkySimStep(&m_context);
}

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

TEST_GROUP_BASE(strbRegister, pinkySimBase)
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


/* STR - Register
   Encoding: 0101 010 Rm:3 Rn:3 Rt:3 */
TEST(strbRegister, UseAMixOfRegistersWordAligned)
{
    emitInstruction16("0101010mmmnnnttt", R7, R3, R0);
    setRegisterValue(R3, INITIAL_PC);
    setRegisterValue(R7, 4);
    SimpleMemory_SetMemory(m_context.pMemory, INITIAL_PC + 4, 0xBAADFEED, READ_WRITE);
    pinkySimStep(&m_context);
    CHECK_EQUAL(0xBAADFE00, IMemory_Read32(m_context.pMemory, INITIAL_PC + 4));
}

TEST(strbRegister, UseAnotherMixOfRegistersSecondByteInWord)
{
    emitInstruction16("0101010mmmnnnttt", R1, R0, R7);
    setRegisterValue(R0, INITIAL_PC);
    setRegisterValue(R1, 5);
    SimpleMemory_SetMemory(m_context.pMemory, INITIAL_PC + 4, 0xBAADFEED, READ_WRITE);
    pinkySimStep(&m_context);
    CHECK_EQUAL(0xBAAD77ED, IMemory_Read32(m_context.pMemory, INITIAL_PC + 4));
}

TEST(strbRegister, YetAnotherMixOfRegistersThirdByteInWord)
{
    emitInstruction16("0101010mmmnnnttt", R0, R7, R4);
    setRegisterValue(R7, INITIAL_PC);
    setRegisterValue(R0, 6);
    SimpleMemory_SetMemory(m_context.pMemory, INITIAL_PC + 4, 0xBAADFEED, READ_WRITE);
    pinkySimStep(&m_context);
    CHECK_EQUAL(0xBA44FEED, IMemory_Read32(m_context.pMemory, INITIAL_PC + 4));
}

TEST(strbRegister, YetAnotherMixOfRegistersFourthByteInWord)
{
    emitInstruction16("0101010mmmnnnttt", R0, R7, R5);
    setRegisterValue(R7, INITIAL_PC);
    setRegisterValue(R0, 7);
    SimpleMemory_SetMemory(m_context.pMemory, INITIAL_PC + 4, 0xBAADFEED, READ_WRITE);
    pinkySimStep(&m_context);
    CHECK_EQUAL(0x55ADFEED, IMemory_Read32(m_context.pMemory, INITIAL_PC + 4));
}

TEST(strbRegister, AttemptStoreToInvalidAddress)
{
    emitInstruction16("0101010mmmnnnttt", R7, R3, R0);
    setRegisterValue(R3, 0xFFFFFFFC);
    setRegisterValue(R7, 0);
    setExpectedStepReturn(PINKYSIM_STEP_HARDFAULT);
    setExpectedRegisterValue(PC, INITIAL_PC);
    pinkySimStep(&m_context);
}

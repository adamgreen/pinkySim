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

TEST_GROUP_BASE(ldrsbRegister, pinkySimBase)
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


/* LDRSB - Register
   Encoding: 0101 011 Rm:3 Rn:3 Rt:3 */
TEST(ldrsbRegister, UseAMixOfRegistersWordAligned)
{
    emitInstruction16("0101011mmmnnnttt", R7, R3, R0);
    setRegisterValue(R3, INITIAL_PC);
    setRegisterValue(R7, 4);
    SimpleMemory_SetMemory(m_context.pMemory, INITIAL_PC + 4, 0xBAADFEED, READ_ONLY);
    setExpectedRegisterValue(R0, 0xFFFFFFED);
    pinkySimStep(&m_context);
}

TEST(ldrsbRegister, UseAnotherMixOfRegistersSecondByteInWord)
{
    emitInstruction16("0101011mmmnnnttt", R1, R0, R7);
    setRegisterValue(R0, INITIAL_PC);
    setRegisterValue(R1, 5);
    SimpleMemory_SetMemory(m_context.pMemory, INITIAL_PC + 4, 0xBAADFEED, READ_ONLY);
    setExpectedRegisterValue(R7, 0xFFFFFFFE);
    pinkySimStep(&m_context);
}

TEST(ldrsbRegister, YetAnotherMixOfRegistersThirdByteInWord)
{
    emitInstruction16("0101011mmmnnnttt", R0, R7, R4);
    setRegisterValue(R7, INITIAL_PC);
    setRegisterValue(R0, 6);
    SimpleMemory_SetMemory(m_context.pMemory, INITIAL_PC + 4, 0xBAADFEED, READ_ONLY);
    setExpectedRegisterValue(R4, 0xFFFFFFAD);
    pinkySimStep(&m_context);
}

TEST(ldrsbRegister, YetAnotherMixOfRegistersFourthByteInWord)
{
    emitInstruction16("0101011mmmnnnttt", R0, R7, R5);
    setRegisterValue(R7, INITIAL_PC);
    setRegisterValue(R0, 7);
    SimpleMemory_SetMemory(m_context.pMemory, INITIAL_PC + 4, 0xBAADFEED, READ_ONLY);
    setExpectedRegisterValue(R5, 0xFFFFFFBA);
    pinkySimStep(&m_context);
}

TEST(ldrsbRegister, LoadAPositiveValue)
{
    emitInstruction16("0101011mmmnnnttt", R7, R3, R0);
    setRegisterValue(R3, INITIAL_PC);
    setRegisterValue(R7, 4);
    SimpleMemory_SetMemory(m_context.pMemory, INITIAL_PC + 4, 0xFFFFFF7F, READ_ONLY);
    setExpectedRegisterValue(R0, 0x7F);
    pinkySimStep(&m_context);
}

TEST(ldrsbRegister, AttemptLoadInvalidAddress)
{
    emitInstruction16("0101011mmmnnnttt", R7, R3, R0);
    setRegisterValue(R3, 0xFFFFFFFC);
    setRegisterValue(R7, 0);
    setExpectedStepReturn(PINKYSIM_STEP_HARDFAULT);
    setExpectedRegisterValue(PC, INITIAL_PC);
    pinkySimStep(&m_context);
}

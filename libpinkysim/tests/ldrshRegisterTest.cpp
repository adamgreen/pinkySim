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

TEST_GROUP_BASE(ldrshRegister, pinkySimBase)
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


/* LDRSH - Register
   Encoding: 0101 111 Rm:3 Rn:3 Rt:3 */
TEST(ldrshRegister, UseAMixOfRegistersWordAligned)
{
    emitInstruction16("0101111mmmnnnttt", R7, R3, R0);
    setRegisterValue(R3, INITIAL_PC);
    setRegisterValue(R7, 4);
    SimpleMemory_SetMemory(m_context.pMemory, INITIAL_PC + 4, 0xBAADFEED, READ_ONLY);
    setExpectedRegisterValue(R0, 0xFFFFFEED);
    pinkySimStep(&m_context);
}

TEST(ldrshRegister, UseAnotherMixOfRegistersWordAligned)
{
    emitInstruction16("0101111mmmnnnttt", R1, R0, R7);
    setRegisterValue(R0, INITIAL_PC);
    setRegisterValue(R1, 4);
    SimpleMemory_SetMemory(m_context.pMemory, INITIAL_PC + 4, 0xBAADFEED, READ_ONLY);
    setExpectedRegisterValue(R7, 0xFFFFFEED);
    pinkySimStep(&m_context);
}

TEST(ldrshRegister, YetAnotherMixOfRegistersNotWordAligned)
{
    emitInstruction16("0101111mmmnnnttt", R0, R7, R4);
    setRegisterValue(R7, INITIAL_PC);
    setRegisterValue(R0, 6);
    SimpleMemory_SetMemory(m_context.pMemory, INITIAL_PC + 4, 0xBAADFEED, READ_ONLY);
    setExpectedRegisterValue(R4, 0xFFFFBAAD);
    pinkySimStep(&m_context);
}

TEST(ldrshRegister, LoadPositiveHalfWord)
{
    emitInstruction16("0101111mmmnnnttt", R0, R7, R4);
    setRegisterValue(R7, INITIAL_PC);
    setRegisterValue(R0, 4);
    SimpleMemory_SetMemory(m_context.pMemory, INITIAL_PC + 4, 0xFFFF7FFF, READ_ONLY);
    setExpectedRegisterValue(R4, 0x7FFF);
    pinkySimStep(&m_context);
}

TEST(ldrshRegister, AttemptUnalignedLoad)
{
    emitInstruction16("0101111mmmnnnttt", R7, R3, R0);
    setRegisterValue(R3, INITIAL_PC);
    setRegisterValue(R7, 1);
    setExpectedStepReturn(PINKYSIM_STEP_HARDFAULT);
    setExpectedRegisterValue(PC, INITIAL_PC);
    pinkySimStep(&m_context);
}

TEST(ldrshRegister, AttemptLoadFromInvalidAddress)
{
    emitInstruction16("0101111mmmnnnttt", R7, R3, R0);
    setRegisterValue(R3, 0xFFFFFFFC);
    setRegisterValue(R7, 0);
    setExpectedStepReturn(PINKYSIM_STEP_HARDFAULT);
    setExpectedRegisterValue(PC, INITIAL_PC);
    pinkySimStep(&m_context);
}

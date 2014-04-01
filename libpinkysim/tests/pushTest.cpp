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

TEST_GROUP_BASE(push, pinkySimBase)
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


/* PUSH
   Encoding: 1011 0 10 M:1 RegisterList:8 */
TEST(push, JustPushLR)
{
    emitInstruction16("1011010Mrrrrrrrr", 1, 0);
    setExpectedRegisterValue(SP, INITIAL_SP - 4);
    SimpleMemory_SetMemory(m_context.pMemory, INITIAL_SP - 4, 0x0, READ_WRITE);
    pinkySimStep(&m_context);
    CHECK_EQUAL(INITIAL_LR, IMemory_Read32(m_context.pMemory, INITIAL_SP - 4));
}

TEST(push, JustPushR0)
{
    emitInstruction16("1011010Mrrrrrrrr", 0, 1);
    setExpectedRegisterValue(SP, INITIAL_SP - 4);
    SimpleMemory_SetMemory(m_context.pMemory, INITIAL_SP - 4, 0xFFFFFFFF, READ_WRITE);
    pinkySimStep(&m_context);
    CHECK_EQUAL(0x0, IMemory_Read32(m_context.pMemory, INITIAL_SP - 4));
}

TEST(push, JustPushR7)
{
    emitInstruction16("1011010Mrrrrrrrr", 0, 1 << 7);
    setExpectedRegisterValue(SP, INITIAL_SP - 4);
    SimpleMemory_SetMemory(m_context.pMemory, INITIAL_SP - 4, 0xFFFFFFFF, READ_WRITE);
    pinkySimStep(&m_context);
    CHECK_EQUAL(0x77777777, IMemory_Read32(m_context.pMemory, INITIAL_SP - 4));
}

TEST(push, PushAll)
{
    emitInstruction16("1011010Mrrrrrrrr", 1, 0xFF);
    setExpectedRegisterValue(SP, INITIAL_SP - 4 * 9);
    for (int i = 1 ; i <= 9 ; i++)
        SimpleMemory_SetMemory(m_context.pMemory, INITIAL_SP - 4 * i, 0xFFFFFFFF, READ_WRITE);
    pinkySimStep(&m_context);
    for (int i = 0 ; i < 8 ; i++)
        CHECK_EQUAL(0x11111111 * i, IMemory_Read32(m_context.pMemory, INITIAL_SP - 4 * (9 - i)));
    CHECK_EQUAL(INITIAL_LR, IMemory_Read32(m_context.pMemory, INITIAL_SP - 4));
}

TEST_SIM_ONLY(push, HardFaultFromInvalidMemoryWrite)
{
    emitInstruction16("1011010Mrrrrrrrr", 0, 1);
    setRegisterValue(SP, 0xFFFFFFFC);
    setExpectedStepReturn(PINKYSIM_STEP_HARDFAULT);
    setExpectedRegisterValue(PC, INITIAL_PC);
    pinkySimStep(&m_context);
}

TEST_SIM_ONLY(push, UnpredictableToPushNoRegisters)
{
    emitInstruction16("1011010Mrrrrrrrr", 0, 0);
    setExpectedStepReturn(PINKYSIM_STEP_UNPREDICTABLE);
    setExpectedRegisterValue(PC, INITIAL_PC);
    pinkySimStep(&m_context);
}

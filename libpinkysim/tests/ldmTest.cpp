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

TEST_GROUP_BASE(ldm, pinkySimBase)
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


/* LDM
   Encoding: 1100 1 Rn:3 RegisterList:8 */
TEST(ldm, JustPopR0WithR7AsAddress_WritebackNewAddressToR7)
{
    emitInstruction16("11001nnnrrrrrrrr", R7, (1 << 0));
    setRegisterValue(R7, INITIAL_PC + 16);
    setExpectedRegisterValue(R7, INITIAL_PC + 16 + 1 * 4);
    setExpectedRegisterValue(R0, 0xFFFFFFFF);
    SimpleMemory_SetMemory(m_context.pMemory, INITIAL_PC + 16, 0xFFFFFFFF, READ_ONLY);
    pinkySimStep(&m_context);
}

TEST(ldm, JustPopR7WithR0AsAddress_WritebackNewAddressToR0)
{
    emitInstruction16("11001nnnrrrrrrrr", R0, (1 << 7));
    setRegisterValue(R0, INITIAL_PC + 16);
    setExpectedRegisterValue(R0, INITIAL_PC + 16 + 1 * 4);
    setExpectedRegisterValue(R7, 0xFFFFFFFF);
    SimpleMemory_SetMemory(m_context.pMemory, INITIAL_PC + 16, 0xFFFFFFFF, READ_ONLY);
    pinkySimStep(&m_context);
}

TEST(ldm, PopAllNoWriteback)
{
    emitInstruction16("11001nnnrrrrrrrr", R0, 0xFF);
    setRegisterValue(R0, INITIAL_PC + 16);
    setExpectedRegisterValue(R0, 0);
    setExpectedRegisterValue(R1, 1);
    setExpectedRegisterValue(R2, 2);
    setExpectedRegisterValue(R3, 3);
    setExpectedRegisterValue(R4, 4);
    setExpectedRegisterValue(R5, 5);
    setExpectedRegisterValue(R6, 6);
    setExpectedRegisterValue(R7, 7);
    for (int i = 0 ; i < 8 ; i++)
        SimpleMemory_SetMemory(m_context.pMemory, INITIAL_PC + 16 + 4 * i, i, READ_ONLY);
    pinkySimStep(&m_context);
}

TEST(ldm, PopAllButAddressRegister_WritebackNewAddress)
{
    emitInstruction16("11001nnnrrrrrrrr", R7, 0x7F);
    setRegisterValue(R7, INITIAL_PC + 16);
    setExpectedRegisterValue(R0, 0);
    setExpectedRegisterValue(R1, 1);
    setExpectedRegisterValue(R2, 2);
    setExpectedRegisterValue(R3, 3);
    setExpectedRegisterValue(R4, 4);
    setExpectedRegisterValue(R5, 5);
    setExpectedRegisterValue(R6, 6);
    setExpectedRegisterValue(R7, INITIAL_PC + 16 + 7 * 4);
    for (int i = 0 ; i < 7 ; i++)
        SimpleMemory_SetMemory(m_context.pMemory, INITIAL_PC + 16 + 4 * i, i, READ_ONLY);
    pinkySimStep(&m_context);
}

TEST(ldm, HardFaultFromInvalidMemoryRead)
{
    emitInstruction16("11001nnnrrrrrrrr", 0, (1 << 0));
    setRegisterValue(R0, 0xFFFFFFFC);
    setExpectedStepReturn(PINKYSIM_STEP_HARDFAULT);
    setExpectedRegisterValue(PC, INITIAL_PC);
    pinkySimStep(&m_context);
}

TEST(ldm, UnpredictableToPopNoRegisters)
{
    emitInstruction16("11001nnnrrrrrrrr", 0, 0);
    setExpectedStepReturn(PINKYSIM_STEP_UNPREDICTABLE);
    setExpectedRegisterValue(PC, INITIAL_PC);
    pinkySimStep(&m_context);
}

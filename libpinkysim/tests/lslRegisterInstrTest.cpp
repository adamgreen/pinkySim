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


TEST_GROUP_BASE(lslRegister, pinkySimBase)
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


/* LSL - Register (Logical Shift Left)
   Encoding: 010000 0010 Rm:3 Rdn:3 */
TEST(lslRegister, ShiftR7by0_MinimumShift_CarryShouldBeUnmodified)
{
    emitInstruction16("0100000010mmmddd", R0, R7);
    setExpectedXPSRflags("nzC");
    setCarry();
    setExpectedRegisterValue(R7, 0x77777777U);
    pinkySimStep(&m_context);
}

TEST(lslRegister, ShiftValue1by31_NegativeResult)
{
    emitInstruction16("0100000010mmmddd", R4, R3);
    setExpectedXPSRflags("Nzc");
    setRegisterValue(R3, 1);
    setRegisterValue(R4, 31);
    setExpectedRegisterValue(R3, 1 << 31);
    pinkySimStep(&m_context);
}

TEST(lslRegister, ShiftValue1by32_CarryOutFromLowestBit)
{
    emitInstruction16("0100000010mmmddd", R7, R0);
    setExpectedXPSRflags("nZC");
    setRegisterValue(R0, 1);
    setRegisterValue(R7, 32);
    setExpectedRegisterValue(R0, 0);
    pinkySimStep(&m_context);
}

TEST(lslRegister, ShiftNegativeValueBy1_CarryOutFromHighestBit)
{
    emitInstruction16("0100000010mmmddd", R3, R4);
    setExpectedXPSRflags("NzC");
    setRegisterValue(R4, -1);
    setRegisterValue(R3, 1);
    setExpectedRegisterValue(R4, -1 << 1);
    pinkySimStep(&m_context);
}

TEST(lslRegister, ShiftValue1by33_NoCarry)
{
    emitInstruction16("0100000010mmmddd", R7, R0);
    setExpectedXPSRflags("nZc");
    setRegisterValue(R0, 1);
    setRegisterValue(R7, 33);
    setExpectedRegisterValue(R0, 0);
    pinkySimStep(&m_context);
}

TEST(lslRegister, ShiftValuee1by255_MaximumShift)
{
    emitInstruction16("0100000010mmmddd", R7, R0);
    setExpectedXPSRflags("nZc");
    setRegisterValue(R0, 1);
    setRegisterValue(R7, 255);
    setExpectedRegisterValue(R0, 0);
    pinkySimStep(&m_context);
}

TEST(lslRegister, ShiftValue1by256_ShouldBeTreatedAs0Shift_CarryUnmodified)
{
    emitInstruction16("0100000010mmmddd", R7, R0);
    setExpectedXPSRflags("nzc");
    clearCarry();
    setRegisterValue(R0, 1);
    setRegisterValue(R7, 256);
    setExpectedRegisterValue(R0, 1);
    pinkySimStep(&m_context);
}

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


TEST_GROUP_BASE(asrRegister, pinkySimBase)
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


/* ASR - Register (Arithmetic Shift Right)
   Encoding: 010000 0100 Rm:3 Rdn:3 */
TEST(asrRegister, Shift1by1_CarryOutFromLowestBit)
{
    emitInstruction16("0100000100mmmddd", R0, R7);
    setRegisterValue(R7, 1);
    setRegisterValue(R0, 1);
    setExpectedXPSRflags("nZC");
    setExpectedRegisterValue(R7, (int32_t)1U >> 1U);
    pinkySimStep(&m_context);
}

TEST(asrRegister, Shift1by0_MinimumShift_CarryUnmodified)
{
    emitInstruction16("0100000100mmmddd", R0, R7);
    setRegisterValue(R7, 1);
    setRegisterValue(R0, 0);
    setExpectedXPSRflags("nz");
    setExpectedRegisterValue(R7, (int32_t)1 >> 0);
    pinkySimStep(&m_context);
}

TEST(asrRegister, Shift2by1_NoCarryFromLowestBit)
{
    emitInstruction16("0100000100mmmddd", R3, R2);
    setRegisterValue(R2, 2);
    setRegisterValue(R3, 1);
    setExpectedXPSRflags("nzc");
    setExpectedRegisterValue(R2, (int32_t)2 >> 1);
    pinkySimStep(&m_context);
}

TEST(asrRegister, ShiftNegativeNumberby31)
{
    emitInstruction16("0100000100mmmddd", R3, R2);
    setRegisterValue(R2, -1);
    setRegisterValue(R3, 31);
    setExpectedXPSRflags("NzC");
    setExpectedRegisterValue(R2, (int32_t)-1 >> 31);
    pinkySimStep(&m_context);
}

TEST(asrRegister, ShiftMaximumNegativeValueBy32_CarryOutFromHighestBit)
{
    emitInstruction16("0100000100mmmddd", R7, R0);
    setRegisterValue(R0, 0x80000000);
    setRegisterValue(R7, 32);
    setExpectedXPSRflags("NzC");
    setExpectedRegisterValue(R0, -1);
    pinkySimStep(&m_context);
}

TEST(asrRegister, ShiftNegativeValueby33)
{
    emitInstruction16("0100000100mmmddd", R3, R2);
    setRegisterValue(R2, -1);
    setRegisterValue(R3, 33);
    setExpectedXPSRflags("NzC");
    setExpectedRegisterValue(R2, -1);
    pinkySimStep(&m_context);
}

TEST(asrRegister, ShiftPositiveValueby33)
{
    emitInstruction16("0100000100mmmddd", R3, R2);
    setRegisterValue(R2, 0x7FFFFFFF);
    setRegisterValue(R3, 33);
    setExpectedXPSRflags("nZc");
    setExpectedRegisterValue(R2, 0);
    pinkySimStep(&m_context);
}

TEST(asrRegister, ShiftNegativeValueByMaximumShiftOf255)
{
    emitInstruction16("0100000100mmmddd", R3, R2);
    setRegisterValue(R2, -1);
    setRegisterValue(R3, 255);
    setExpectedXPSRflags("NzC");
    setExpectedRegisterValue(R2, -1);
    pinkySimStep(&m_context);
}

TEST(asrRegister, ShiftOf256ShouldBeTreatedAsShiftOf0_CarryUnmodified)
{
    emitInstruction16("0100000100mmmddd", R7, R0);
    setRegisterValue(R0, -1);
    setRegisterValue(R7, 256);
    setExpectedXPSRflags("Nz");
    setExpectedRegisterValue(R0, (int32_t)-1 >> 0);
    pinkySimStep(&m_context);
}

TEST(asrRegister, ShiftLargestPositiveNumberBy31)
{
    emitInstruction16("0100000100mmmddd", R2, R3);
    setRegisterValue(R3, 0x7FFFFFFF);
    setRegisterValue(R2, 31);
    setExpectedXPSRflags("nZC");
    setExpectedRegisterValue(R3, (int32_t)0x7FFFFFFF >> 31);
    pinkySimStep(&m_context);
}

TEST(asrRegister, ShiftLargestNegativeNumberBy1)
{
    emitInstruction16("0100000100mmmddd", R2, R3);
    setRegisterValue(R3, 0x80000000);
    setRegisterValue(R2, 1);
    setExpectedXPSRflags("Nzc");
    setExpectedRegisterValue(R3, (int32_t)0x80000000 >> 1);
    pinkySimStep(&m_context);
}

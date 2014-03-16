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


TEST_GROUP_BASE(lsrRegister, pinkySimBase)
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


/* LSR - Register (Logical Shift Right)
   Encoding: 010000 0011 Rm:3 Rdn:3 */
TEST(lsrRegister, ShiftR7byR0WithValues1and1_CarryOutFromLSB)
{
    emitInstruction16("0100000011mmmddd", R0, R7);
    setRegisterValue(R0, 1);
    setRegisterValue(R7, 1);
    setExpectedXPSRflags("nZC");
    setExpectedRegisterValue(R7, 0);
    pinkySimStep(&m_context);
}

TEST(lsrRegister, ShiftR7byR0WithValues1and0_MinimumShift_CarryUnmodified)
{
    emitInstruction16("0100000011mmmddd", R0, R7);
    setRegisterValue(R7, 1);
    setRegisterValue(R0, 0);
    setExpectedXPSRflags("nz");
    setExpectedRegisterValue(R7, 1);
    pinkySimStep(&m_context);
}

TEST(lsrRegister, ShiftR2byR3WithValues2and1_NoCarry)
{
    emitInstruction16("0100000011mmmddd", R3, R2);
    setRegisterValue(R2, 2);
    setRegisterValue(R3, 1);
    setExpectedXPSRflags("nzc");
    setExpectedRegisterValue(R2, 2 >> 1);
    pinkySimStep(&m_context);
}

TEST(lsrRegister, ShiftR2byR3WithShiftOf31)
{
    emitInstruction16("0100000011mmmddd", R3, R2);
    setRegisterValue(R2, -1);
    setRegisterValue(R3, 31);
    setExpectedXPSRflags("nzC");
    setExpectedRegisterValue(R2, -1U >> 31);
    pinkySimStep(&m_context);
}

TEST(lsrRegister, ShiftR0byR7WithShiftOf32_CarryOutMSB)
{
    emitInstruction16("0100000011mmmddd", R7, R0);
    setRegisterValue(R0, 0x80000000);
    setRegisterValue(R7, 32);
    setExpectedXPSRflags("nZC");
    setExpectedRegisterValue(R0, 0);
    pinkySimStep(&m_context);
}

TEST(lsrRegister, ShiftR2byR3WithShiftOf33)
{
    emitInstruction16("0100000011mmmddd", R3, R2);
    setRegisterValue(R2, -1);
    setRegisterValue(R3, 33);
    setExpectedXPSRflags("nZc");
    setExpectedRegisterValue(R2, 0);
    pinkySimStep(&m_context);
}

TEST(lsrRegister, ShiftR2byR3WithMaximumShiftOf255)
{
    emitInstruction16("0100000011mmmddd", R3, R2);
    setRegisterValue(R2, -1);
    setRegisterValue(R3, 255);
    setExpectedXPSRflags("nZc");
    setExpectedRegisterValue(R2, 0);
    pinkySimStep(&m_context);
}

TEST(lsrRegister, ShiftR0byR7WithShiftOf256_ShouldBeTreatedAs0Shift_CarryUnmodified)
{
    emitInstruction16("0100000011mmmddd", R7, R0);
    setRegisterValue(R0, -1);
    setRegisterValue(R7, 256);
    setExpectedXPSRflags("Nz");
    setExpectedRegisterValue(R0, -1);
    pinkySimStep(&m_context);
}

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


TEST_GROUP_BASE(rorRegister, pinkySimBase)
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


/* ROR - Register (ROtate Right)
   Encoding: 010000 0111 Rm:3 Rdn:3 */
TEST(rorRegister, Rotate1by1_CarryOutFromLowestBit)
{
    emitInstruction16("0100000111mmmddd", R0, R7);
    setExpectedXPSRflags("NzC");
    setRegisterValue(R0, 1);
    setRegisterValue(R7, 1);
    setExpectedRegisterValue(R7, 0x80000000);
    pinkySimStep(&m_context);
}

TEST(rorRegister, Rotate1by0_MinimumShift_CarryUnmodified)
{
    emitInstruction16("0100000111mmmddd", R7, R0);
    setExpectedXPSRflags("nz");
    setRegisterValue(R0, 1);
    setRegisterValue(R7, 0);
    setExpectedRegisterValue(R0, 1);
    pinkySimStep(&m_context);
}

TEST(rorRegister, Rotate2by1_NoCarry)
{
    emitInstruction16("0100000111mmmddd", R3, R2);
    setExpectedXPSRflags("nzc");
    setRegisterValue(R2, 2);
    setRegisterValue(R3, 1);
    setExpectedRegisterValue(R2, 2 >> 1);
    pinkySimStep(&m_context);
}

TEST(rorRegister, Rotate16Bits)
{
    emitInstruction16("0100000111mmmddd", R3, R2);
    setExpectedXPSRflags("nzc");
    setRegisterValue(R2, 0x12345678);
    setRegisterValue(R3, 16);
    setExpectedRegisterValue(R2, 0x56781234);
    pinkySimStep(&m_context);
}

TEST(rorRegister, RotateWithShiftOf31)
{
    emitInstruction16("0100000111mmmddd", R3, R2);
    setExpectedXPSRflags("nzc");
    setRegisterValue(R2, 0x80000000);
    setRegisterValue(R3, 31);
    setExpectedRegisterValue(R2, 0x00000001);
    pinkySimStep(&m_context);
}

TEST(rorRegister, RotateBy32_CarryOutHighestBit)
{
    emitInstruction16("0100000111mmmddd", R7, R0);
    setExpectedXPSRflags("NzC");
    setRegisterValue(R0, 0x80000000);
    setRegisterValue(R7, 32);
    setExpectedRegisterValue(R0, 0x80000000);
    pinkySimStep(&m_context);
}

TEST(rorRegister, RotateBy33)
{
    emitInstruction16("0100000111mmmddd", R3, R2);
    setExpectedXPSRflags("NzC");
    setRegisterValue(R2, 0x80000001);
    setRegisterValue(R3, 33);
    setExpectedRegisterValue(R2, 0xC0000000);
    pinkySimStep(&m_context);
}

TEST(rorRegister, RotateWithMaximumShiftOf255)
{
    emitInstruction16("0100000111mmmddd", R3, R2);
    setExpectedXPSRflags("nzc");
    setRegisterValue(R2, 0x80000000);
    setRegisterValue(R3, 255);
    setExpectedRegisterValue(R2, 0x00000001);
    pinkySimStep(&m_context);
}

TEST(rorRegister, RotateWithShiftOf256_ShouldBeTreatedAs0Shift_CarryUnmodified)
{
    emitInstruction16("0100000111mmmddd", R7, R0);
    setExpectedXPSRflags("Nz");
    setRegisterValue(R0, 0x80000000);
    setRegisterValue(R7, 256);
    setExpectedRegisterValue(R0, 0x80000000);
    pinkySimStep(&m_context);
}

TEST(rorRegister, Rotate0by16)
{
    emitInstruction16("0100000111mmmddd", R7, R0);
    setExpectedXPSRflags("nZc");
    setRegisterValue(R0, 0);
    setRegisterValue(R7, 16);
    setExpectedRegisterValue(R0, 0);
    pinkySimStep(&m_context);
}

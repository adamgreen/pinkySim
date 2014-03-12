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
TEST(lslRegister, ShiftR7byR0_MinimumShift_CarryUnmodified)
{
    emitInstruction16("0100000010mmmddd", R0, R7);
    setExpectedAPSRflags("nz");
    setExpectedRegisterValue(R7, 0x77777777U);
    pinkySimStep(&m_context);
}

TEST(lslRegister, ShiftR3byR4WithValues1and31_NegativeResult)
{
    emitInstruction16("0100000010mmmddd", R4, R3);
    setRegisterValue(R3, 1);
    setRegisterValue(R4, 31);
    setExpectedAPSRflags("Nzc");
    setExpectedRegisterValue(R3, 1 << 31);
    pinkySimStep(&m_context);
}

TEST(lslRegister, ShiftR0byR7WithValues1and32_CarryOutFromLSB)
{
    emitInstruction16("0100000010mmmddd", R7, R0);
    setRegisterValue(R0, 1);
    setRegisterValue(R7, 32);
    setExpectedAPSRflags("nZC");
    setExpectedRegisterValue(R0, 0);
    pinkySimStep(&m_context);
}

TEST(lslRegister, ShiftR0byR7WithValues1and33_NoCarry)
{
    emitInstruction16("0100000010mmmddd", R7, R0);
    setRegisterValue(R0, 1);
    setRegisterValue(R7, 33);
    setExpectedAPSRflags("nZc");
    setExpectedRegisterValue(R0, 0);
    pinkySimStep(&m_context);
}

TEST(lslRegister, ShiftR0byR7WithValues1and255_MaximumShift)
{
    emitInstruction16("0100000010mmmddd", R7, R0);
    setRegisterValue(R0, 1);
    setRegisterValue(R7, 255);
    setExpectedAPSRflags("nZc");
    setExpectedRegisterValue(R0, 0);
    pinkySimStep(&m_context);
}

TEST(lslRegister, ShiftR0byR7WithValues1and256_ShouldBeTreatedAs0Shift)
{
    emitInstruction16("0100000010mmmddd", R7, R0);
    setRegisterValue(R0, 1);
    setRegisterValue(R7, 256);
    setExpectedAPSRflags("nz");
    setExpectedRegisterValue(R0, 1);
    pinkySimStep(&m_context);
}

TEST(lslRegister, ShiftR4byR3_CarryOutFromMSB)
{
    emitInstruction16("0100000010mmmddd", R3, R4);
    setRegisterValue(R4, -1);
    setRegisterValue(R3, 1);
    setExpectedAPSRflags("NzC");
    setExpectedRegisterValue(R4, -1 << 1);
    pinkySimStep(&m_context);
}

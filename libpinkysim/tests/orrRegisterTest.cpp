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

TEST_GROUP_BASE(orrRegister, pinkySimBase)
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


/* ORR - Register
   Encoding: 010000 1100 Rm:3 Rdn:3 */
TEST(orrRegister, UseLowestRegisterForBothArgs)
{
    emitInstruction16("0100001100mmmddd", R0, R0);
    setExpectedXPSRflags("nZc");
    setExpectedRegisterValue(R0, 0);
    clearCarry();
    pinkySimStep(&m_context);
}

TEST(orrRegister, UseHighestRegisterForBothArgs)
{
    emitInstruction16("0100001100mmmddd", R7, R7);
    setExpectedXPSRflags("nzC");
    setCarry();
    pinkySimStep(&m_context);
}

TEST(orrRegister, OrR3andR7)
{
    emitInstruction16("0100001100mmmddd", R3, R7);
    setExpectedXPSRflags("nz");
    setExpectedRegisterValue(R7, 0x33333333 | 0x77777777);
    pinkySimStep(&m_context);
}

TEST(orrRegister, UseOrToTurnOnNegativeSignBit)
{
    emitInstruction16("0100001100mmmddd", R7, R0);
    setRegisterValue(R0, 0x7FFFFFFF);
    setRegisterValue(R7, 0x80000000);
    setExpectedXPSRflags("Nz");
    setExpectedRegisterValue(R0, 0x7FFFFFFF | 0x80000000);
    pinkySimStep(&m_context);
}

TEST(orrRegister, HaveAndResultNotBeSameAsEitherSource)
{
    emitInstruction16("0100001100mmmddd", R7, R0);
    setRegisterValue(R0, 0x12345678);
    setRegisterValue(R7, 0xF0F0F0F0);
    setExpectedXPSRflags("Nz");
    setExpectedRegisterValue(R0, 0xF2F4F6F8);
    pinkySimStep(&m_context);
}
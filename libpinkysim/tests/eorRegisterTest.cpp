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

TEST_GROUP_BASE(eorRegister, pinkySimBase)
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


/* EOR - Register
   Encoding: 010000 0001 Rm:3 Rdn:3 */
/* NOTE: APSR_C state is maintained by this instruction. */
TEST(eorRegister, UseLowestRegisterForBothArgsAndResultShouldBeZero)
{
    emitInstruction16("0100000001mmmddd", R0, R0);
    setExpectedXPSRflags("nZc");
    setExpectedRegisterValue(R0, 0);
    clearCarry();
    pinkySimStep(&m_context);
}

TEST(eorRegister, UseHighestRegisterForBothArgsAndResultShouldBeZero)
{
    emitInstruction16("0100000001mmmddd", R7, R7);
    setExpectedXPSRflags("nZC");
    setExpectedRegisterValue(R7, 0);
    setCarry();
    pinkySimStep(&m_context);
}

TEST(eorRegister, XorR3andR7)
{
    emitInstruction16("0100000001mmmddd", R3, R7);
    setExpectedXPSRflags("nzc");
    setExpectedRegisterValue(R7, 0x33333333 ^ 0x77777777);
    clearCarry();
    pinkySimStep(&m_context);
}

TEST(eorRegister, UseXorToJustFlipNegativeSignBitOn)
{
    emitInstruction16("0100000001mmmddd", R7, R3);
    setRegisterValue(R7, 0x80000000);
    setExpectedXPSRflags("NzC");
    setExpectedRegisterValue(R3, 0x33333333 ^ 0x80000000);
    setCarry();
    pinkySimStep(&m_context);
}

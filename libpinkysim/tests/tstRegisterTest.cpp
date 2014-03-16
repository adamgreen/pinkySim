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

TEST_GROUP_BASE(tstRegister, pinkySimBase)
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


/* TST - Register
   Encoding: 010000 1000 Rm:3 Rn:3 */
TEST(tstRegister, UseLowestRegisterForBothArgsAndResultShouldBeZero)
{
    emitInstruction16("0100001000mmmnnn", R0, R0);
    setExpectedXPSRflags("nZ");
    pinkySimStep(&m_context);
}

TEST(tstRegister, UseHighestRegisterForBothArgsAndRegisterWillBeUnchanged)
{
    emitInstruction16("0100001000mmmnnn", R7, R7);
    setExpectedXPSRflags("nz");
    pinkySimStep(&m_context);
}

TEST(tstRegister, AndR3andR7)
{
    emitInstruction16("0100001000mmmnnn", R3, R7);
    setExpectedXPSRflags("nz");
    pinkySimStep(&m_context);
}

TEST(tstRegister, UseAndToJustKeepNegativeSignBit)
{
    emitInstruction16("0100001000mmmnnn", R7, R0);
    setRegisterValue(R0, -1);
    setRegisterValue(R7, 0x80000000);
    setExpectedXPSRflags("Nz");
    pinkySimStep(&m_context);
}

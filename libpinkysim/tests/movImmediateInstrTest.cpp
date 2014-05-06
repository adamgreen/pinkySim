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


TEST_GROUP_BASE(movImmediate, pinkySimBase)
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


/* MOV - Immediate
   Encoding: 001 00 Rd:3 Imm:8 */
/* NOTE: APSR_C state is maintained by this instruction. */
TEST(movImmediate, MovToR0)
{
    emitInstruction16("00100dddiiiiiiii", R0, 127);
    // Use a couple of tests to explicitly set/clear carry to verify both states are maintained.
    setExpectedXPSRflags("nzc");
    clearCarry();
    setExpectedRegisterValue(R0, 127);
    pinkySimStep(&m_context);
}

TEST(movImmediate, MovToR7)
{
    emitInstruction16("00100dddiiiiiiii", R7, 127);
    setExpectedXPSRflags("nzC");
    setCarry();
    setExpectedRegisterValue(R7, 127);
    pinkySimStep(&m_context);
}

TEST(movImmediate, MovSmallestImmediateValueToR3)
{
    emitInstruction16("00100dddiiiiiiii", R3, 0);
    setExpectedXPSRflags("nZ");
    setExpectedRegisterValue(R3, 0);
    pinkySimStep(&m_context);
}

TEST(movImmediate, MovLargestImmediateValueToR3)
{
    emitInstruction16("00100dddiiiiiiii", R3, 255);
    setExpectedXPSRflags("nz");
    setExpectedRegisterValue(R3, 255);
    pinkySimStep(&m_context);
}

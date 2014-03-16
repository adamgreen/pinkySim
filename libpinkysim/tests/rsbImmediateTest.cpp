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

TEST_GROUP_BASE(rsbImmediate, pinkySimBase)
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


/* RSB - Immediate
   Encoding: 010000 1001 Rn:3 Rd:3 */
TEST(rsbImmediate, UseLowestRegisterOnly)
{
    emitInstruction16("0100001001nnnddd", R0, R0);
    setExpectedXPSRflags("nZCv");
    setExpectedRegisterValue(R0, 0U);
    pinkySimStep(&m_context);
}

TEST(rsbImmediate, UseHigestRegisterOnly)
{
    emitInstruction16("0100001001nnnddd", R7, R7);
    setExpectedXPSRflags("Nzcv");
    setExpectedRegisterValue(R7, -0x77777777U);
    pinkySimStep(&m_context);
}

TEST(rsbImmediate, UseDifferentRegistersForEachArg)
{
    emitInstruction16("0100001001nnnddd", R2, R0);
    setExpectedXPSRflags("Nzcv");
    setExpectedRegisterValue(R0, -0x22222222);
    pinkySimStep(&m_context);
}

TEST(rsbImmediate, ForceOverflowByNegatingLargestNegativeValue)
{
    emitInstruction16("0100001001nnnddd", R0, R7);
    setExpectedXPSRflags("NzcV");
    setRegisterValue(R0, 0x80000000);
    setExpectedRegisterValue(R7, 0x80000000U);
    pinkySimStep(&m_context);
}

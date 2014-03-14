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

TEST_GROUP_BASE(mul, pinkySimBase)
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


/* MUL
   Encoding: 010000 1101 Rn:3 Rdm:3 */
TEST(mul, UseLowestRegisterForAllArgs)
{
    emitInstruction16("0100001101nnnddd", R0, R0);
    setExpectedAPSRflags("nZ");
    setExpectedRegisterValue(R0, 0U);
    pinkySimStep(&m_context);
}

TEST(mul, UseHigestRegisterForAllArgs)
{
    emitInstruction16("0100001101nnnddd", R7, R7);
    setExpectedAPSRflags("Nz");
    setExpectedRegisterValue(R7, 0x77777777U * 0x77777777U);
    pinkySimStep(&m_context);
}

TEST(mul, UseDifferentRegistersForEachArg)
{
    emitInstruction16("0100001101nnnddd", R1, R2);
    setRegisterValue(R1, 0xA5A5);
    setRegisterValue(R2, 2);
    setExpectedAPSRflags("nz");
    setExpectedRegisterValue(R2, 0xA5A5U << 1U);
    pinkySimStep(&m_context);
}

TEST(mul, MultiplyBy16BitMaximumValues)
{
    emitInstruction16("0100001101nnnddd", R1, R2);
    setRegisterValue(R1, 0xFFFF);
    setRegisterValue(R2, 0xFFFF);
    setExpectedAPSRflags("Nz");
    setExpectedRegisterValue(R2, 0xFFFF * 0xFFFF);
    pinkySimStep(&m_context);
}

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

TEST_GROUP_BASE(cmnRegister, pinkySimBase)
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


/* CMN - Register (Compare Negative)
   Encoding: 010000 1011 Rm:3 Rn:3 */
TEST(cmnRegister, UseR1ForAllArgsResultsInAllFlagsCleared)
{
    emitInstruction16("0100001011mmmnnn", R1, R1);
    setExpectedAPSRflags("nzcv");
    pinkySimStep(&m_context);
}

TEST(cmnRegister, UseLowestRegisterForAllArgs)
{
    emitInstruction16("0100001011mmmnnn", R0, R0);
    setExpectedAPSRflags("nZcv");
    pinkySimStep(&m_context);
}

TEST(cmnRegister, UseHigestRegister)
{
    emitInstruction16("0100001011mmmnnn", R7, R7);
    setExpectedAPSRflags("NzcV");
    pinkySimStep(&m_context);
}

TEST(cmnRegister, UseDifferentRegistersForEachArg)
{
    emitInstruction16("0100001011mmmnnn", R1, R2);
    setExpectedAPSRflags("nzcv");
    pinkySimStep(&m_context);
}

TEST(cmnRegister, ForceCarryWithNoOverflow)
{
    emitInstruction16("0100001011mmmnnn", R1, R2);
    setExpectedAPSRflags("nZCv");
    setRegisterValue(R1, -1);
    setRegisterValue(R2, 1);
    pinkySimStep(&m_context);
}

TEST(cmnRegister, ForceCarryAndOverflow)
{
    emitInstruction16("0100001011mmmnnn", R1, R2);
    setExpectedAPSRflags("nzCV");
    setRegisterValue(R1, -1);
    setRegisterValue(R2, 0x80000000U);
    pinkySimStep(&m_context);
}

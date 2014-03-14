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

TEST_GROUP_BASE(cmpRegister, pinkySimBase)
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


/* SUB - Register - Encoding T1
   Encoding: 010000 1010 Rm:3 Rn:3 */
TEST(cmpRegister, UseLowestRegisterForAllArgs)
{
    emitInstruction16("0100001010mmmnnn", R0, R0);
    setExpectedAPSRflags("nZCv");
    pinkySimStep(&m_context);
}

TEST(cmpRegister, UseHigestRegisterForAllArgs)
{
    emitInstruction16("0100001010mmmnnn", R7, R7);
    setExpectedAPSRflags("nZCv");
    pinkySimStep(&m_context);
}

TEST(cmpRegister, RnLargerThanRm)
{
    emitInstruction16("0100001010mmmnnn", R1, R2);
    setExpectedAPSRflags("nzCv");
    pinkySimStep(&m_context);
}

TEST(cmpRegister, RnSmallerThanRm)
{
    emitInstruction16("0100001010mmmnnn", R1, R0);
    setExpectedAPSRflags("Nzcv");
    setRegisterValue(R1, 1);
    pinkySimStep(&m_context);
}

TEST(cmpRegister, ForceNegativeOverflow)
{
    emitInstruction16("0100001010mmmnnn", R1, R2);
    setExpectedAPSRflags("nzCV");
    setRegisterValue(R2, 0x80000000U);
    setRegisterValue(R1, 1U);
    pinkySimStep(&m_context);
}

TEST(cmpRegister, ForcePositiveOverflow)
{
    emitInstruction16("0100001010mmmnnn", R1, R2);
    setExpectedAPSRflags("NzcV");
    setRegisterValue(R2, 0x7FFFFFFFU);
    setRegisterValue(R1, -1U);
    pinkySimStep(&m_context);
}
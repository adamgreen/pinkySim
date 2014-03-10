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

TEST_GROUP_BASE(addRegister, pinkySimBase)
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


/* ADD - Register
   Encoding: 000 11 0 0 Rm:3 Rn:3 Rd:3 */
TEST(addRegister, UseR1ForAllArgsAndCarryOverflowZeroNegativeFlagsShouldBeClear)
{
    emitInstruction16("0001100mmmnnnddd", R1, R1, R1);
    setExpectedAPSRflags("nzcv");
    setExpectedRegisterValue(R1, 0x11111111U + 0x11111111U);
    pinkySimStep(&m_context);
}

TEST(addRegister, UseLowestRegisterForAllArgsAndOnlyZeroFlagShouldBeSet)
{
    emitInstruction16("0001100mmmnnnddd", R0, R0, R0);
    setExpectedAPSRflags("nZcv");
    setExpectedRegisterValue(R0, 0U);
    pinkySimStep(&m_context);
}

TEST(addRegister, UseHigestRegisterForAllArgsAndWillBeNegativeBecauseOfOverflow)
{
    emitInstruction16("0001100mmmnnnddd", R7, R7, R7);
    setExpectedAPSRflags("NzcV");
    setExpectedRegisterValue(R7, 0x77777777U + 0x77777777U);
    pinkySimStep(&m_context);
}

TEST(addRegister, UseDifferentRegistersForEachArg)
{
    emitInstruction16("0001100mmmnnnddd", R1, R2, R0);
    setExpectedAPSRflags("nzcv");
    setExpectedRegisterValue(R0, 0x11111111U + 0x22222222U);
    pinkySimStep(&m_context);
}

TEST(addRegister, ForceCarryWithNoOverflow)
{
    emitInstruction16("0001100mmmnnnddd", R1, R2, R0);
    setExpectedAPSRflags("nZCv");
    setRegisterValue(R1, -1);
    setRegisterValue(R2, 1);
    setExpectedRegisterValue(R0, -1 + 1);
    pinkySimStep(&m_context);
}

TEST(addRegister, ForceCarryAndOverflow)
{
    emitInstruction16("0001100mmmnnnddd", R1, R2, R0);
    setExpectedAPSRflags("nzCV");
    setRegisterValue(R1, -1);
    setRegisterValue(R2, 0x80000000U);
    setExpectedRegisterValue(R0, -1 + (int32_t)0x80000000U);
    pinkySimStep(&m_context);
}

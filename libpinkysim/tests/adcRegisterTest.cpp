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

TEST_GROUP_BASE(adcRegister, pinkySimBase)
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


/* ADC - Register (ADD with Carry)
   Encoding: 010000 0101 Rm:3 Rdn:3 */
TEST(adcRegister, UseR1ForAllArgsAndCarryOverflowZeroNegativeFlagsShouldBeClear)
{
    emitInstruction16("0100000101mmmddd", R1, R1);
    setExpectedAPSRflags("nzcv");
    setExpectedRegisterValue(R1, 0x11111111U + 0x11111111U);
    clearCarry();
    pinkySimStep(&m_context);
}

TEST(adcRegister, UseLowestRegisterForAllArgsAndOnlyZeroFlagShouldBeSet)
{
    emitInstruction16("0100000101mmmddd", R0, R0);
    setExpectedAPSRflags("nZcv");
    setExpectedRegisterValue(R0, 0U);
    clearCarry();
    pinkySimStep(&m_context);
}

TEST(adcRegister, Add0to0WithCarryInSetToGiveAResultOf1)
{
    emitInstruction16("0100000101mmmddd", R0, R0);
    setExpectedAPSRflags("nzcv");
    setExpectedRegisterValue(R0, 0U + 0U + 1U);
    setCarry();
    pinkySimStep(&m_context);
}

TEST(adcRegister, UseHigestRegisterForAllArgsAndWillBeNegativeBecauseOfOverflow)
{
    emitInstruction16("0100000101mmmddd", R7, R7);
    setExpectedAPSRflags("NzcV");
    setExpectedRegisterValue(R7, 0x77777777U + 0x77777777U);
    clearCarry();
    pinkySimStep(&m_context);
}

TEST(adcRegister, UseDifferentRegistersForEachArg)
{
    emitInstruction16("0100000101mmmddd", R1, R2);
    setExpectedAPSRflags("nzcv");
    setExpectedRegisterValue(R2, 0x11111111U + 0x22222222U);
    clearCarry();
    pinkySimStep(&m_context);
}

TEST(adcRegister, ForceCarryWithNoOverflow)
{
    emitInstruction16("0100000101mmmddd", R1, R2);
    setExpectedAPSRflags("nZCv");
    setRegisterValue(R1, -1);
    setRegisterValue(R2, 1);
    setExpectedRegisterValue(R2, -1 + 1);
    clearCarry();
    pinkySimStep(&m_context);
}

TEST(adcRegister, ForceCarryAndOverflow)
{
    emitInstruction16("0100000101mmmddd", R1, R2);
    setExpectedAPSRflags("nzCV");
    setRegisterValue(R1, -1);
    setRegisterValue(R2, 0x80000000U);
    setExpectedRegisterValue(R2, -1 + (int32_t)0x80000000U);
    clearCarry();
    pinkySimStep(&m_context);
}

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

TEST_GROUP_BASE(subImmediate, pinkySimBase)
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


/* SUB - Immediate - Encoding T1
   Encoding: 000 11 1 1 Imm:3 Rn:3 Rd:3 */
TEST(subImmediate, T1UseLowestRegisterOnlySubtractSmallestImmediateCausesZeroResultAndCarryToIndicateNoBorrow)
{
    emitInstruction16("0001111iiinnnddd", 0, R0, R0);
    setExpectedAPSRflags("nZCv");
    setExpectedRegisterValue(R0, 0U);
    pinkySimStep(&m_context);
}

TEST(subImmediate, T1UseHigestRegisterOnlySubtractLargestImmediateCausesCarryToIndicateNoBorrow)
{
    emitInstruction16("0001111iiinnnddd", 7, R7, R7);
    setExpectedAPSRflags("nzCv");
    setExpectedRegisterValue(R7, 0x77777777U - 7U);
    pinkySimStep(&m_context);
}

TEST(subImmediate, T1UseDifferentRegistersForEachArgSubtract3From0CausesNoCarryToIndicateBorrowAndNegativeResult)
{
    emitInstruction16("0001111iiinnnddd", 3, R0, R2);
    setExpectedAPSRflags("Nzcv");
    setExpectedRegisterValue(R2, 0U - 3U);
    pinkySimStep(&m_context);
}

TEST(subImmediate, T1ForceOverflowPastLargestNegativeInteger)
{
    emitInstruction16("0001111iiinnnddd", 1, R0, R7);
    setExpectedAPSRflags("nzCV");
    setRegisterValue(R0, 0x80000000);
    setExpectedRegisterValue(R7, 0x80000000U - 1U);
    pinkySimStep(&m_context);
}



/* SUB - Immediate - Encoding T2
   Encoding: 001 11 Rdn:3 Imm:8 */
TEST(subImmediate, T2UseLowestRegisterToSubtractSmallestImmediateCausesZeroResultAndCarryToIndicateNoBorrow)
{
    emitInstruction16("00111dddiiiiiiii", R0, 0);
    setExpectedAPSRflags("nZCv");
    setExpectedRegisterValue(R0, 0U);
    pinkySimStep(&m_context);
}

TEST(subImmediate, T2UseHigestRegisterToSubtractLargestImmediateCausesCarryToIndicateNoBorrow)
{
    emitInstruction16("00111dddiiiiiiii", R7, 255);
    setExpectedAPSRflags("nzCv");
    setExpectedRegisterValue(R7, 0x77777777U - 255U);
    pinkySimStep(&m_context);
}

TEST(subImmediate, T2Subtract127FromR0CausesNoCarryToIndicateBorrowAndNegativeResult)
{
    emitInstruction16("00111dddiiiiiiii", R0, 127);
    setExpectedAPSRflags("Nzcv");
    setExpectedRegisterValue(R0, 0U - 127U);
    pinkySimStep(&m_context);
}

TEST(subImmediate, T2ForceOverflowPastLargestNegativeInteger)
{
    emitInstruction16("00111dddiiiiiiii", R0, 1);
    setExpectedAPSRflags("nzCV");
    setRegisterValue(R0, 0x80000000);
    setExpectedRegisterValue(R0, 0x80000000U - 1U);
    pinkySimStep(&m_context);
}

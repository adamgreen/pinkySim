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

TEST_GROUP_BASE(bx, pinkySimBase)
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


/* BX (Branch and Exchange)
   Encoding: 010001 11 0 Rm:4 (0)(0)(0) */
TEST(bx, UseLowestRegisterToBranchToEvenAddressWhichClearsThumbModeToCauseHardFaultOnNextInstruction)
{
    emitInstruction16("010001110mmmm000", R0);
    setExpectedXPSRflags("t");
    setExpectedRegisterValue(PC, 0x0);
    pinkySimStep(&m_context);
    
    setExpectedStepReturn(PINKYSIM_STEP_HARDFAULT);
    pinkySimStep(&m_context);
}

TEST(bx, UseHighestRegisterToBranchToOddAddressWhichIsRequiredForThumb)
{
    emitInstruction16("010001110mmmm000", LR);
    setRegisterValue(LR, (INITIAL_PC + 16) | 1);
    setExpectedRegisterValue(PC, INITIAL_PC + 16);
    pinkySimStep(&m_context);
}

TEST(bx, UnpredictableToUseR15)
{
    emitInstruction16("010001110mmmm000", PC);
    setExpectedStepReturn(PINKYSIM_STEP_UNPREDICTABLE);
    pinkySimStep(&m_context);
}

TEST(bx, UnpredictableForBit0ToBeHigh)
{
    emitInstruction16("010001110mmmm001", R0);
    setExpectedStepReturn(PINKYSIM_STEP_UNPREDICTABLE);
    pinkySimStep(&m_context);
}

TEST(bx, UnpredictableForBit1ToBeHigh)
{
    emitInstruction16("010001110mmmm010", R0);
    setExpectedStepReturn(PINKYSIM_STEP_UNPREDICTABLE);
    pinkySimStep(&m_context);
}

TEST(bx, UnpredictableForBit2ToBeHigh)
{
    emitInstruction16("010001110mmmm100", R0);
    setExpectedStepReturn(PINKYSIM_STEP_UNPREDICTABLE);
    pinkySimStep(&m_context);
}

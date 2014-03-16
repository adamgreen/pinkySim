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

TEST_GROUP_BASE(blx, pinkySimBase)
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


/* BLX (Branch with Link and Exchange)
   Encoding: 010001 11 1 Rm:4 (0)(0)(0) */
TEST(blx, UseLowestRegisterToBranchToEvenAddressWhichClearsThumbMode)
{
    emitInstruction16("010001111mmmm000", R0);
    setExpectedXPSRflags("t");
    setExpectedRegisterValue(PC, 0x0);
    setExpectedRegisterValue(LR, (INITIAL_PC + 2) | 1);
    pinkySimStep(&m_context);
    
    setExpectedStepReturn(PINKYSIM_STEP_HARDFAULT);
    pinkySimStep(&m_context);
}

TEST(blx, UseHighestRegisterToBranchToOddAddressWhichIsOk)
{
    emitInstruction16("010001111mmmm000", LR);
    setRegisterValue(LR, 0x1);
    setExpectedRegisterValue(PC, 0x0);
    setExpectedRegisterValue(LR, (INITIAL_PC + 2) | 1);
    pinkySimStep(&m_context);

    // UNDONE: Could place a BKPT insstruction at branch destination.
    //pinkySimStep(&m_context);
}

TEST(blx, UnpredictableToUseR15)
{
    emitInstruction16("010001111mmmm000", PC);
    setExpectedStepReturn(PINKYSIM_STEP_UNPREDICTABLE);
    pinkySimStep(&m_context);
}

TEST(blx, UnpredictableForBit0ToBeHigh)
{
    emitInstruction16("010001111mmmm001", R0);
    setExpectedStepReturn(PINKYSIM_STEP_UNPREDICTABLE);
    pinkySimStep(&m_context);
}

TEST(blx, UnpredictableForBit1ToBeHigh)
{
    emitInstruction16("010001111mmmm010", R0);
    setExpectedStepReturn(PINKYSIM_STEP_UNPREDICTABLE);
    pinkySimStep(&m_context);
}

TEST(blx, UnpredictableForBit2ToBeHigh)
{
    emitInstruction16("010001111mmmm100", R0);
    setExpectedStepReturn(PINKYSIM_STEP_UNPREDICTABLE);
    pinkySimStep(&m_context);
}

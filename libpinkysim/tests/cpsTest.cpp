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

TEST_GROUP_BASE(cps, pinkySimBase)
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


/* CPS
   Encoding: 1011 0110 011 im:1 (0)(0)(1)(0) */
TEST_SIM_ONLY(cps, InterruptEnable)
{
    emitInstruction16("10110110011i0010", 0);
    m_context.PRIMASK |= PRIMASK_PM;
    pinkySimStep(&m_context);
    CHECK_FALSE(m_context.PRIMASK & PRIMASK_PM);
}

TEST_SIM_ONLY(cps, InterruptDisable)
{
    emitInstruction16("10110110011i0010", 1);
    m_context.PRIMASK &= ~PRIMASK_PM;
    pinkySimStep(&m_context);
    CHECK_TRUE(m_context.PRIMASK & PRIMASK_PM);
}

TEST_SIM_ONLY(cps, UnpredictableBecauseOfBit0)
{
    emitInstruction16("10110110011i0011", 0);
    setExpectedStepReturn(PINKYSIM_STEP_UNPREDICTABLE);
    setExpectedRegisterValue(PC, INITIAL_PC);
    pinkySimStep(&m_context);
}

TEST_SIM_ONLY(cps, UnpredictableBecauseOfBit1)
{
    emitInstruction16("10110110011i0000", 0);
    setExpectedStepReturn(PINKYSIM_STEP_UNPREDICTABLE);
    setExpectedRegisterValue(PC, INITIAL_PC);
    pinkySimStep(&m_context);
}
TEST_SIM_ONLY(cps, UnpredictableBecauseOfBit2)
{
    emitInstruction16("10110110011i0110", 0);
    setExpectedStepReturn(PINKYSIM_STEP_UNPREDICTABLE);
    setExpectedRegisterValue(PC, INITIAL_PC);
    pinkySimStep(&m_context);
}

TEST_SIM_ONLY(cps, UnpredictableBecauseOfBit3)
{
    emitInstruction16("10110110011i1010", 0);
    setExpectedStepReturn(PINKYSIM_STEP_UNPREDICTABLE);
    setExpectedRegisterValue(PC, INITIAL_PC);
    pinkySimStep(&m_context);
}

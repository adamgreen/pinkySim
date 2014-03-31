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

TEST_GROUP_BASE(pinkySim, pinkySimBase)
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


TEST(pinkySim, Undedfined16BitWithAllZeroesForImmedaite)
{
    emitInstruction16("11011110iiiiiiii", 0);
    setExpectedStepReturn(PINKYSIM_STEP_UNDEFINED);
    setExpectedRegisterValue(PC, INITIAL_PC);
    pinkySimStep(&m_context);
}

TEST(pinkySim, Undedfined16BitWithAllOnesForImmedaite)
{
    emitInstruction16("11011110iiiiiiii", -1);
    setExpectedStepReturn(PINKYSIM_STEP_UNDEFINED);
    setExpectedRegisterValue(PC, INITIAL_PC);
    pinkySimStep(&m_context);
}

TEST(pinkySim, Undefined32BitWithAllZeroesForImmediate)
{
    emitInstruction32("111101111111iiii", "1010iiiiiiiiiiii", 0, 0);
    setExpectedStepReturn(PINKYSIM_STEP_UNDEFINED);
    setExpectedRegisterValue(PC, INITIAL_PC);
    pinkySimStep(&m_context);
}

TEST(pinkySim, Undefined32BitWithAllOnesForImmediate)
{
    emitInstruction32("111101111111iiii", "1010iiiiiiiiiiii", -1, -1);
    setExpectedStepReturn(PINKYSIM_STEP_UNDEFINED);
    setExpectedRegisterValue(PC, INITIAL_PC);
    pinkySimStep(&m_context);
}

TEST(pinkySim, Encoding1ThatShouldProduceUnpredictableError)
{
    emitInstruction16("0100010100000000");
    setExpectedStepReturn(PINKYSIM_STEP_UNPREDICTABLE);
    setExpectedRegisterValue(PC, INITIAL_PC);
    pinkySimStep(&m_context);
}

TEST(pinkySim, Encoding2ThatShouldProduceUnpredictableError)
{
    emitInstruction16("0100010100111111");
    setExpectedStepReturn(PINKYSIM_STEP_UNPREDICTABLE);
    setExpectedRegisterValue(PC, INITIAL_PC);
    pinkySimStep(&m_context);
}

TEST(pinkySim, HintEncodingsThatShouldProduceUndefinedError)
{
    // opA = XXXX opB != 0
    for (uint32_t opA = 0 ; opA < 16 ; opA++)
    {
        for (uint32_t opB = 1 ; opB < 16 ; opB++)
        {
            emitInstruction16("10111111aaaabbbb", opA, opB);
            setExpectedStepReturn(PINKYSIM_STEP_UNDEFINED);
            setExpectedRegisterValue(PC, INITIAL_PC);
            pinkySimStep(&m_context);
            initContext();
        }
    }
}

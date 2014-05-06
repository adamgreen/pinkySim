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

TEST_GROUP_BASE(undefined, pinkySimBase)
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


TEST(undefined, Undedfined16BitWithAllZeroesForImmedaite)
{
    emitInstruction16("11011110iiiiiiii", 0);
    setExpectedStepReturn(PINKYSIM_STEP_UNDEFINED);
    setExpectedRegisterValue(PC, INITIAL_PC);
    pinkySimStep(&m_context);
}

TEST(undefined, Undedfined16BitWithAllOnesForImmedaite)
{
    emitInstruction16("11011110iiiiiiii", -1);
    setExpectedStepReturn(PINKYSIM_STEP_UNDEFINED);
    setExpectedRegisterValue(PC, INITIAL_PC);
    pinkySimStep(&m_context);
}

TEST(undefined, Undefined32BitWithAllZeroesForImmediate)
{
    emitInstruction32("111101111111iiii", "1010iiiiiiiiiiii", 0, 0);
    setExpectedStepReturn(PINKYSIM_STEP_UNDEFINED);
    setExpectedRegisterValue(PC, INITIAL_PC);
    pinkySimStep(&m_context);
}

TEST(undefined, Undefined32BitWithAllOnesForImmediate)
{
    emitInstruction32("111101111111iiii", "1010iiiiiiiiiiii", -1, -1);
    setExpectedStepReturn(PINKYSIM_STEP_UNDEFINED);
    setExpectedRegisterValue(PC, INITIAL_PC);
    pinkySimStep(&m_context);
}

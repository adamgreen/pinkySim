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

TEST_GROUP_BASE(dmb, pinkySimBase)
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


/* DMB
   Encoding: 11110 0 111 01 1 (1)(1)(1)(1)
             10 (0) 0 (1)(1)(1)(1) 0101 option:4 */
TEST(dmb, OptionSetTo15)
{
    emitInstruction32("1111001110111111", "100011110101oooo", 15);
    pinkySimStep(&m_context);
}

TEST(dmb, OptionSetTo0)
{
    emitInstruction32("1111001110111111", "100011110101oooo", 0);
    pinkySimStep(&m_context);
}

TEST(dmb, UnpredictableBecauseOfBit1_0)
{
    emitInstruction32("1111001110111110", "100011110101oooo", 15);
    setExpectedStepReturn(PINKYSIM_STEP_UNPREDICTABLE);
    pinkySimStep(&m_context);
}

TEST(dmb, UnpredictableBecauseOfBit1_1)
{
    emitInstruction32("1111001110111101", "100011110101oooo", 15);
    setExpectedStepReturn(PINKYSIM_STEP_UNPREDICTABLE);
    pinkySimStep(&m_context);
}

TEST(dmb, UnpredictableBecauseOfBit1_2)
{
    emitInstruction32("1111001110111011", "100011110101oooo", 15);
    setExpectedStepReturn(PINKYSIM_STEP_UNPREDICTABLE);
    pinkySimStep(&m_context);
}

TEST(dmb, UnpredictableBecauseOfBit1_3)
{
    emitInstruction32("1111001110110111", "100011110101oooo", 15);
    setExpectedStepReturn(PINKYSIM_STEP_UNPREDICTABLE);
    pinkySimStep(&m_context);
}

TEST(dmb, UnpredictableBecauseOfBit2_8)
{
    emitInstruction32("1111001110111111", "100011100101oooo", 15);
    setExpectedStepReturn(PINKYSIM_STEP_UNPREDICTABLE);
    pinkySimStep(&m_context);
}

TEST(dmb, UnpredictableBecauseOfBit2_9)
{
    emitInstruction32("1111001110111111", "100011010101oooo", 15);
    setExpectedStepReturn(PINKYSIM_STEP_UNPREDICTABLE);
    pinkySimStep(&m_context);
}

TEST(dmb, UnpredictableBecauseOfBit2_10)
{
    emitInstruction32("1111001110111111", "100010110101oooo", 15);
    setExpectedStepReturn(PINKYSIM_STEP_UNPREDICTABLE);
    pinkySimStep(&m_context);
}

TEST(dmb, UnpredictableBecauseOfBit2_11)
{
    emitInstruction32("1111001110111111", "100001110101oooo", 15);
    setExpectedStepReturn(PINKYSIM_STEP_UNPREDICTABLE);
    pinkySimStep(&m_context);
}

TEST(dmb, UnpredictableBecauseOfBit2_13)
{
    emitInstruction32("1111001110111111", "101011110101oooo", 15);
    setExpectedStepReturn(PINKYSIM_STEP_UNPREDICTABLE);
    pinkySimStep(&m_context);
}

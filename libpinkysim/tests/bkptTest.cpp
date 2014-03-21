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

TEST_GROUP_BASE(bkpt, pinkySimBase)
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


/* BKPT
   Encoding: 1011 1110 Imm:8 */
TEST(bkpt, SmallestImmediate)
{
    emitInstruction16("10111110iiiiiiii", 0);
    setExpectedRegisterValue(PC, INITIAL_PC);
    setExpectedStepReturn(PINKYSIM_STEP_BKPT);
    pinkySimStep(&m_context);
}

TEST(bkpt, LargestImmediate)
{
    emitInstruction16("10111110iiiiiiii", 255);
    setExpectedRegisterValue(PC, INITIAL_PC);
    setExpectedStepReturn(PINKYSIM_STEP_BKPT);
    pinkySimStep(&m_context);
}

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


TEST(pinkySim, InvalidOpcodeShouldProduceUndefinedError)
{
    emitInstruction16("1101111000000000");
    setExpectedStepReturn(PINKYSIM_STEP_UNDEFINED);
    pinkySimStep(&m_context);
}

TEST(pinkySim, Encoding1ThatShouldProduceUnpredictableError)
{
    emitInstruction16("0100010100000000");
    setExpectedStepReturn(PINKYSIM_STEP_UNPREDICTABLE);
    pinkySimStep(&m_context);
}

TEST(pinkySim, Encoding2ThatShouldProduceUnpredictableError)
{
    emitInstruction16("0100010100111111");
    setExpectedStepReturn(PINKYSIM_STEP_UNPREDICTABLE);
    pinkySimStep(&m_context);
}

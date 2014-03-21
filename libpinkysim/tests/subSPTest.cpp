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

TEST_GROUP_BASE(subSP, pinkySimBase)
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


/* SUB SP Minus Immediate
   Encoding: 1011 0000 1 Imm:7 */
TEST(subSP, SmallestImmediate)
{
    emitInstruction16("101100001iiiiiii", 0);
    setExpectedRegisterValue(SP, INITIAL_SP - 0);
    pinkySimStep(&m_context);
}

TEST(subSP, LargestImmediate)
{
    emitInstruction16("101100001iiiiiii", 127);
    setExpectedRegisterValue(SP, INITIAL_SP - 127 * 4);
    pinkySimStep(&m_context);
}

TEST(subSP, UseIntermediateValues)
{
    emitInstruction16("101100001iiiiiii", 64);
    setExpectedRegisterValue(SP, INITIAL_SP - 64 * 4);
    pinkySimStep(&m_context);
}

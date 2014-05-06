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

TEST_GROUP_BASE(addSP, pinkySimBase)
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


/* ADD SP Plus Immediate - Encoding T1
   Encoding: 1010 1 Rd:3 Imm:8 */
TEST(addSP, T1UseHighestRegisterAddSmallestImmediate)
{
    emitInstruction16("10101dddiiiiiiii", R7, 0);
    setExpectedRegisterValue(R7, INITIAL_SP + 0);
    pinkySimStep(&m_context);
}

TEST(addSP, T1UseLowestRegisterAddLargestImmediate)
{
    emitInstruction16("10101dddiiiiiiii", R0, 255);
    setExpectedRegisterValue(R0, INITIAL_SP + 255 * 4);
    pinkySimStep(&m_context);
}

TEST(addSP, T1UseIntermediateValues)
{
    emitInstruction16("10101dddiiiiiiii", R3, 128);
    setExpectedRegisterValue(R3, INITIAL_SP + 128 * 4);
    pinkySimStep(&m_context);
}



/* ADD SP Plus Immediate - Encoding T2
   Encoding: 1011 0000 0 Imm:7 */
TEST(addSP, T2SmallestImmediate)
{
    emitInstruction16("101100000iiiiiii", 0);
    setRegisterValue(SP, INITIAL_PC + 1024);
    setExpectedRegisterValue(SP, INITIAL_PC + 1024 + 0);
    pinkySimStep(&m_context);
}

TEST(addSP, T2LargestImmediate)
{
    emitInstruction16("101100000iiiiiii", 127);
    setRegisterValue(SP, INITIAL_PC + 1024);
    setExpectedRegisterValue(SP, INITIAL_PC + 1024 + 127 * 4);
    pinkySimStep(&m_context);
}

TEST(addSP, T2IntermediateValues)
{
    emitInstruction16("101100000iiiiiii", 64);
    setRegisterValue(SP, INITIAL_PC + 1024);
    setExpectedRegisterValue(SP, INITIAL_PC + 1024 + 64 * 4);
    pinkySimStep(&m_context);
}

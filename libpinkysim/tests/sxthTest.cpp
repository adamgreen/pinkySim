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

TEST_GROUP_BASE(sxth, pinkySimBase)
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


/* SXTH (Sign ExTend Halfword)
   Encoding: 1011 0010 00 Rm:3 Rd:3 */
TEST(sxth, SignExtendLowestRegisterIntoHighestRegister_PositiveValue)
{
    emitInstruction16("1011001000mmmddd", R7, R0);
    setRegisterValue(R7, 0x7FFF);
    setExpectedRegisterValue(R0, 0x7FFF);
    pinkySimStep(&m_context);
}

TEST(sxth, SignExtendHighestRegisterIntoLowestRegister_NegativeValue)
{
    emitInstruction16("1011001000mmmddd", R0, R7);
    setRegisterValue(R0, 0x8000);
    setExpectedRegisterValue(R7, 0xFFFF8000);
    pinkySimStep(&m_context);
}

TEST(sxth, OverwriteUpperBits_PositiveValue)
{
    emitInstruction16("1011001000mmmddd", R6, R1);
    setRegisterValue(R6, 0xF00D7FFF);
    setExpectedRegisterValue(R1, 0x00007FFF);
    pinkySimStep(&m_context);
}

TEST(sxth, OverwriteUpperBits_NegativeValue)
{
    emitInstruction16("1011001000mmmddd", R2, R5);
    setRegisterValue(R2, 0xF00D8000);
    setExpectedRegisterValue(R5, 0xFFFF8000);
    pinkySimStep(&m_context);
}

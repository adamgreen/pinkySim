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

TEST_GROUP_BASE(uxtb, pinkySimBase)
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


/* UXTB (Unsigned ExTend Byte)
   Encoding: 1011 0010 11 Rm:3 Rd:3 */
TEST(uxtb, ExtendLowestRegisterIntoHighestRegister_PositiveValue)
{
    emitInstruction16("1011001011mmmddd", R7, R0);
    setRegisterValue(R7, 0x7F);
    setExpectedRegisterValue(R0, 0x7F);
    pinkySimStep(&m_context);
}

TEST(uxtb, ExtendHighestRegisterIntoLowestRegister_NegativeValue)
{
    emitInstruction16("1011001011mmmddd", R0, R7);
    setRegisterValue(R0, 0x80);
    setExpectedRegisterValue(R7, 0x80);
    pinkySimStep(&m_context);
}

TEST(uxtb, OverwriteUpperBits_PositiveValue)
{
    emitInstruction16("1011001011mmmddd", R6, R1);
    setRegisterValue(R6, 0xBADBAD7F);
    setExpectedRegisterValue(R1, 0x7F);
    pinkySimStep(&m_context);
}

TEST(uxtb, OverwriteUpperBits_NegativeValue)
{
    emitInstruction16("1011001011mmmddd", R2, R5);
    setRegisterValue(R2, 0xBADBAD80);
    setExpectedRegisterValue(R5, 0x80);
    pinkySimStep(&m_context);
}

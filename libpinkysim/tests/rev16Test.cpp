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

TEST_GROUP_BASE(rev16, pinkySimBase)
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


/* REV16
   Encoding: 1011 1010 01 Rm:3 Rd:3 */
TEST(rev16, RevR0toR7)
{
    emitInstruction16("1011101001mmmddd", R0, R7);
    setRegisterValue(R0, 0x12345678);
    setExpectedRegisterValue(R7, 0x34127856);
    pinkySimStep(&m_context);
}

TEST(rev16, RevR7toR0)
{
    emitInstruction16("1011101001mmmddd", R7, R0);
    setRegisterValue(R7, 0x12345678);
    setExpectedRegisterValue(R0, 0x34127856);
    pinkySimStep(&m_context);
}

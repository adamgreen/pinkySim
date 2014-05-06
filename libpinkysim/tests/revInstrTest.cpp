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

TEST_GROUP_BASE(rev, pinkySimBase)
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


/* REV
   Encoding: 1011 1010 00 Rm:3 Rd:3 */
TEST(rev, RevR0toR7)
{
    emitInstruction16("1011101000mmmddd", R0, R7);
    setRegisterValue(R0, 0x12345678);
    setExpectedRegisterValue(R7, 0x78563412);
    pinkySimStep(&m_context);
}

TEST(rev, RevR7toR0)
{
    emitInstruction16("1011101000mmmddd", R7, R0);
    setRegisterValue(R7, 0x12345678);
    setExpectedRegisterValue(R0, 0x78563412);
    pinkySimStep(&m_context);
}

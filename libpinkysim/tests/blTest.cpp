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

TEST_GROUP_BASE(bl, pinkySimBase)
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


/* BLX (Branch with Link)
   Encoding: 11110 S Imm:10
             11 J1:1 1 J2:1 Imm:11 
    Note: J1 and J2 are translated to immediate bits via I1 = NOT(J1 XOR S) */
TEST(bl, OffsetOf0)
{
    emitInstruction32("11110Siiiiiiiiii", "11j1kiiiiiiiiiii", 0, 0, 1, 1, 0);
    setExpectedRegisterValue(PC, INITIAL_PC + 4);
    setExpectedRegisterValue(LR, (INITIAL_PC + 2) | 1);
    pinkySimStep(&m_context);
}

TEST(bl, MaximumPositiveOffset)
{
    emitInstruction32("11110Siiiiiiiiii", "11j1kiiiiiiiiiii", 0, 0x3FF, 0, 0, 0x7FF);
    setExpectedRegisterValue(PC, INITIAL_PC + 4 + 16777214);
    setExpectedRegisterValue(LR, (INITIAL_PC + 2) | 1);
    pinkySimStep(&m_context);
}

TEST(bl, MaximumNegativeOffset)
{
    emitInstruction32("11110Siiiiiiiiii", "11j1kiiiiiiiiiii", 1, 0, 0, 0, 0);
    setExpectedRegisterValue(PC, INITIAL_PC + 4 - 16777216);
    setExpectedRegisterValue(LR, (INITIAL_PC + 2) | 1);
    pinkySimStep(&m_context);
}

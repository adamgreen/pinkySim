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

TEST_GROUP_BASE(nop, pinkySimBase)
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


/* NOP
   Encoding: 1011 1111 0000 0000 */
TEST(nop, BasicTest)
{
    emitInstruction16("1011111100000000");
    pinkySimStep(&m_context);
}



/* Unallocated hint encodings with OpB == 0 are treated as NOP as well. */
TEST(nop, UnallocatedHints)
{
    for (uint32_t opA = 5 ; opA < 16 ; opA++)
    {
        emitInstruction16("10111111aaaa0000", opA);
        pinkySimStep(&m_context);
        initContext();
    }
}

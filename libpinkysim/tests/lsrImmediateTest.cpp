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

// Immediate values used for shift amount in tests.
#define IMM_1  1
#define IMM_32 0


TEST_GROUP_BASE(lsrImmediate, pinkySimBase)
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


/* LSR - Immediate (Logical Shift Right)
   Encoding: 000 01 imm:5 Rm:3 Rd:3 */
TEST(lsrImmediate, R2by1toR0NoCarryNotZeroNotNegative)
{
    emitInstruction16("00001iiiiimmmddd", IMM_1, R2, R0);
    setExpectedXPSRflags("nzc");
    setExpectedRegisterValue(R0, 0x22222222U >> 1);
    pinkySimStep(&m_context);
}

TEST(lsrImmediate, R7by32toR0IsZero)
{
    emitInstruction16("00001iiiiimmmddd", IMM_32, R7, R0);
    setExpectedXPSRflags("nZc");
    setExpectedRegisterValue(R0, 0x0);
    pinkySimStep(&m_context);
}

TEST(lsrImmediate, R1by1toR7HasCarryOut)
{
    emitInstruction16("00001iiiiimmmddd", IMM_1, R1, R7);
    setExpectedXPSRflags("nzC");
    setExpectedRegisterValue(R7, 0x11111111U >> 1); 
    pinkySimStep(&m_context);
}

TEST(lsrImmediate, R0by32HasCarryOutAndIsZero)
{
    emitInstruction16("00001iiiiimmmddd", IMM_32, R0, R0);
    m_context.R[R0] = 0x80000000U;
    setExpectedXPSRflags("nZC");
    setExpectedRegisterValue(R0, 0U);
    pinkySimStep(&m_context);
}

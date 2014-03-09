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
#define IMM_0  0
#define IMM_1  1
#define IMM_3  3
#define IMM_4  4
#define IMM_31 31


TEST_GROUP_BASE(lslImmediate, pinkySimBase)
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


/* LSL - Immediate (Logical Shift Left)
   Encoding: 000 00 imm:5 Rm:3 Rd:3 */
TEST(lslImmediate, MovR7toR0NoCarryNotZeroNotNegative)
{
    emitInstruction16("00000cccccbbbaaa", R0, R7, IMM_0);
    setExpectedAPSRflags("nzc");
    setExpectedRegisterValue(R0, 0x77777777U);
    pinkySimStep(&m_context);
}

TEST(lslImmediate, MovR0toR7IsZero)
{
    emitInstruction16("00000cccccbbbaaa", R7, R0, IMM_0);
    setExpectedAPSRflags("nZc");
    setExpectedRegisterValue(R7, 0x0);
    pinkySimStep(&m_context);
}

TEST(lslImmediate, R1by3toR0IsNegative)
{
    emitInstruction16("00000cccccbbbaaa", R0, R1, IMM_3);
    setExpectedAPSRflags("Nzc");
    setExpectedRegisterValue(R0, 0x11111111U << 3);
    pinkySimStep(&m_context);
}

TEST(lslImmediate, R1by4toR0HasCarryOut)
{
    emitInstruction16("00000cccccbbbaaa", R0, R1, IMM_4);
    setExpectedAPSRflags("nzC");
    setExpectedRegisterValue(R0, 0x11111111U << 4);
    pinkySimStep(&m_context);
}

TEST(lslImmediate, R0by31)
{
    emitInstruction16("00000cccccbbbaaa", R0, R0, IMM_31);
    setExpectedAPSRflags("Nzc");
    setExpectedRegisterValue(R0, 1U << 31);
    m_context.R[R0] = 1U;
    pinkySimStep(&m_context);
}

TEST(lslImmediate, R0by1WithCarryOut)
{
    emitInstruction16("00000cccccbbbaaa", R0, R0, IMM_1);
    setExpectedAPSRflags("nzC");
    setExpectedRegisterValue(R0, 0xA0000000U << 1);
    m_context.R[R0] = 0xA0000000U;
    pinkySimStep(&m_context);
}

TEST(lslImmediate, R0by31WithCarryOut)
{
    emitInstruction16("00000cccccbbbaaa", R0, R0, IMM_31);
    setExpectedAPSRflags("nZC");
    setExpectedRegisterValue(R0, 0x2U << 31);
    m_context.R[R0] = 0x2U;
    pinkySimStep(&m_context);
}

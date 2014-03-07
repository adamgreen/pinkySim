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
    emitInstruction16("00001cccccbbbaaa", R0, R2, IMM_1);
    setXPSRbits(APSR_NZC);
        m_stepReturn = pinkySimStep(&m_context);
    CHECK_EQUAL(PINKYSIM_STEP_OK, m_stepReturn);
    CHECK_EQUAL(0x22222222U >> 1, m_context.R[0]);
    validateUnchangedRegisters(SKIP(R0));
    CHECK_FALSE(m_context.xPSR & APSR_N);
    CHECK_FALSE(m_context.xPSR & APSR_Z);
    CHECK_FALSE(m_context.xPSR & APSR_C);
}

TEST(lsrImmediate, R7by32toR0IsZero)
{
    emitInstruction16("00001cccccbbbaaa", R0, R7, IMM_32);
    setXPSRbits(APSR_NC);
        m_stepReturn = pinkySimStep(&m_context);
    CHECK_EQUAL(PINKYSIM_STEP_OK, m_stepReturn);
    CHECK_EQUAL(0x0, m_context.R[0]);
    validateUnchangedRegisters(SKIP(R0));
    CHECK_FALSE(m_context.xPSR & APSR_N);
    CHECK_TRUE(m_context.xPSR & APSR_Z);
    CHECK_FALSE(m_context.xPSR & APSR_C);
}

TEST(lsrImmediate, R1by1toR7HasCarryOut)
{
    emitInstruction16("00001cccccbbbaaa", R7, R1, IMM_1);
    setXPSRbits(APSR_NZ);
        m_stepReturn = pinkySimStep(&m_context);
    CHECK_EQUAL(PINKYSIM_STEP_OK, m_stepReturn);
    CHECK_EQUAL(0x11111111U >> 1, m_context.R[7]);
    validateUnchangedRegisters(SKIP(R7));
    CHECK_FALSE(m_context.xPSR & APSR_N);
    CHECK_FALSE(m_context.xPSR & APSR_Z);
    CHECK_TRUE(m_context.xPSR & APSR_C);
}

TEST(lsrImmediate, R0by32HasCarryOutAndIsZero)
{
    emitInstruction16("00001cccccbbbaaa", R0, R0, IMM_32);
    m_context.R[R0] = 0x80000000U;
    setXPSRbits(APSR_N);
        m_stepReturn = pinkySimStep(&m_context);
    CHECK_EQUAL(PINKYSIM_STEP_OK, m_stepReturn);
    CHECK_EQUAL(0U, m_context.R[0]);
    validateUnchangedRegisters(SKIP(R0));
    CHECK_FALSE(m_context.xPSR & APSR_N);
    CHECK_TRUE(m_context.xPSR & APSR_Z);
    CHECK_TRUE(m_context.xPSR & APSR_C);
}

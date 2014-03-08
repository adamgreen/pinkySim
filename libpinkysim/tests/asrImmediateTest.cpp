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


TEST_GROUP_BASE(asrImmediate, pinkySimBase)
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


/* ASR - Immediate (Arithmetic Shift Right)
   Encoding: 000 10 imm:5 Rm:3 Rd:3 */
TEST(asrImmediate, R0by1OfNegativeNumber)
{
    emitInstruction16("00010cccccbbbaaa", R0, R0, IMM_1);
    m_context.R[0] = 0x80000000U;
    setXPSRbits(APSR_ZC);
        m_stepReturn = pinkySimStep(&m_context);
    CHECK_EQUAL(PINKYSIM_STEP_OK, m_stepReturn);
    CHECK_EQUAL((int32_t)0x80000000U >> 1, m_context.R[0]);
    validateUnchangedRegisters(SKIP(R0));
    CHECK_TRUE(m_context.xPSR & APSR_N);
    CHECK_FALSE(m_context.xPSR & APSR_Z);
    CHECK_FALSE(m_context.xPSR & APSR_C);
}

TEST(asrImmediate, R0by1OfPositiveNumber)
{
    emitInstruction16("00010cccccbbbaaa", R0, R0, IMM_1);
    m_context.R[0] = 0x7FFFFFFFU;
    setXPSRbits(APSR_NZ);
        m_stepReturn = pinkySimStep(&m_context);
    CHECK_EQUAL(PINKYSIM_STEP_OK, m_stepReturn);
    CHECK_EQUAL((int32_t)0x7FFFFFFFU >> 1, m_context.R[0]);
    validateUnchangedRegisters(SKIP(R0));
    CHECK_FALSE(m_context.xPSR & APSR_N);
    CHECK_FALSE(m_context.xPSR & APSR_Z);
    CHECK_TRUE(m_context.xPSR & APSR_C);
}

TEST(asrImmediate, R0by32OfNegativeNumber)
{
    emitInstruction16("00010cccccbbbaaa", R0, R0, IMM_32);
    m_context.R[0] = 0x80000000U;
    setXPSRbits(APSR_Z);
        m_stepReturn = pinkySimStep(&m_context);
    CHECK_EQUAL(PINKYSIM_STEP_OK, m_stepReturn);
    CHECK_EQUAL(0xFFFFFFFFU, m_context.R[0]);
    validateUnchangedRegisters(SKIP(R0));
    CHECK_TRUE(m_context.xPSR & APSR_N);
    CHECK_FALSE(m_context.xPSR & APSR_Z);
    CHECK_TRUE(m_context.xPSR & APSR_C);
}

TEST(asrImmediate, R0by32OfPositiveNumber)
{
    emitInstruction16("00010cccccbbbaaa", R0, R0, IMM_32);
    m_context.R[0] = 0x7FFFFFFFU;
    setXPSRbits(APSR_NC);
        m_stepReturn = pinkySimStep(&m_context);
    CHECK_EQUAL(PINKYSIM_STEP_OK, m_stepReturn);
    CHECK_EQUAL(0x0U, m_context.R[0]);
    validateUnchangedRegisters(SKIP(R0));
    CHECK_FALSE(m_context.xPSR & APSR_N);
    CHECK_TRUE(m_context.xPSR & APSR_Z);
    CHECK_FALSE(m_context.xPSR & APSR_C);
}

TEST(asrImmediate, R1by1ToR7)
{
    emitInstruction16("00010cccccbbbaaa", R7, R1, IMM_1);
    setXPSRbits(APSR_NZC);
        m_stepReturn = pinkySimStep(&m_context);
    CHECK_EQUAL(PINKYSIM_STEP_OK, m_stepReturn);
    CHECK_EQUAL((int32_t)0x11111111U >> 1, m_context.R[7]);
    validateUnchangedRegisters(SKIP(R7));
    CHECK_FALSE(m_context.xPSR & APSR_N);
    CHECK_FALSE(m_context.xPSR & APSR_Z);
    CHECK_TRUE(m_context.xPSR & APSR_C);
}

TEST(asrImmediate, R7by1ToR0)
{
    emitInstruction16("00010cccccbbbaaa", R0, R7, IMM_1);
    setXPSRbits(APSR_NZC);
        m_stepReturn = pinkySimStep(&m_context);
    CHECK_EQUAL(PINKYSIM_STEP_OK, m_stepReturn);
    CHECK_EQUAL((int32_t)0x77777777U >> 1, m_context.R[0]);
    validateUnchangedRegisters(SKIP(R0));
    CHECK_FALSE(m_context.xPSR & APSR_N);
    CHECK_FALSE(m_context.xPSR & APSR_Z);
    CHECK_TRUE(m_context.xPSR & APSR_C);
}

TEST(asrImmediate, R0by1)
{
    emitInstruction16("00010cccccbbbaaa", R0, R7, IMM_1);
    setXPSRbits(APSR_NZC);
        m_stepReturn = pinkySimStep(&m_context);
    CHECK_EQUAL(PINKYSIM_STEP_OK, m_stepReturn);
    CHECK_EQUAL((int32_t)0x77777777U >> 1, m_context.R[0]);
    validateUnchangedRegisters(SKIP(R0));
    CHECK_FALSE(m_context.xPSR & APSR_N);
    CHECK_FALSE(m_context.xPSR & APSR_Z);
    CHECK_TRUE(m_context.xPSR & APSR_C);
}
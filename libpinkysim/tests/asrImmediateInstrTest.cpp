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
TEST(asrImmediate, ShiftNegativeNumberBy1_Shift0OutFromLowestBit)
{
    emitInstruction16("00010iiiiimmmddd", IMM_1, R0, R7);
    setRegisterValue(R0, 0x80000000U);
    setExpectedXPSRflags("Nzc");
    setExpectedRegisterValue(R7, (int32_t)0x80000000U >> 1);
    pinkySimStep(&m_context);
}

TEST(asrImmediate, ShiftPositiveNumberBy1_Shift1OutFromLowestBit)
{
    emitInstruction16("00010iiiiimmmddd", IMM_1, R7, R0);
    setRegisterValue(R7, 0x7FFFFFFFU);
    setExpectedXPSRflags("nzC");
    setExpectedRegisterValue(R0, (int32_t)0x7FFFFFFFU >> 1);
    pinkySimStep(&m_context);
}

TEST(asrImmediate, NegativeNumberBy32_Shift1OutFromHighestBit)
{
    emitInstruction16("00010iiiiimmmddd", IMM_32, R0, R0);
    setRegisterValue(R0, 0x80000000U);
    setExpectedXPSRflags("NzC");
    setExpectedRegisterValue(R0, 0xFFFFFFFFU);
    pinkySimStep(&m_context);
}

TEST(asrImmediate, PositiveNumberBy32_Shift0FromHighestBit)
{
    emitInstruction16("00010iiiiimmmddd", IMM_32, R1, R6);
    setRegisterValue(R1, 0x7FFFFFFFU);
    setExpectedXPSRflags("nZc");
    setExpectedRegisterValue(R6, 0x0U);
    pinkySimStep(&m_context);
}

TEST(asrImmediate, R1by1ToR7)
{
    emitInstruction16("00010iiiiimmmddd", IMM_1, R1, R7);
    setExpectedXPSRflags("nzC");
    setExpectedRegisterValue(R7, (int32_t)0x11111111U >> 1);
    pinkySimStep(&m_context);
}

TEST(asrImmediate, R7by1ToR2)
{
    emitInstruction16("00010iiiiimmmddd", IMM_1, R7, R2);
    setExpectedXPSRflags("nzC");
    setExpectedRegisterValue(R2, (int32_t)0x77777777U >> 1);
    pinkySimStep(&m_context);
}

TEST(asrImmediate, R0by1)
{
    emitInstruction16("00010iiiiimmmddd", IMM_1, R0, R0);
    setExpectedXPSRflags("nZc");
    setExpectedRegisterValue(R0, (int32_t)0x00000000U >> 1);
    pinkySimStep(&m_context);
}

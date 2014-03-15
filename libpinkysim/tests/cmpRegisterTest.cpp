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

TEST_GROUP_BASE(cmpRegister, pinkySimBase)
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


/* CMP - Register - Encoding T1
   Encoding: 010000 1010 Rm:3 Rn:3 */
TEST(cmpRegister, T1UseLowestRegisterForAllArgs)
{
    emitInstruction16("0100001010mmmnnn", R0, R0);
    setExpectedAPSRflags("nZCv");
    pinkySimStep(&m_context);
}

TEST(cmpRegister, T1UseHigestRegisterForAllArgs)
{
    emitInstruction16("0100001010mmmnnn", R7, R7);
    setExpectedAPSRflags("nZCv");
    pinkySimStep(&m_context);
}

TEST(cmpRegister, T1RnLargerThanRm)
{
    emitInstruction16("0100001010mmmnnn", R1, R2);
    setExpectedAPSRflags("nzCv");
    pinkySimStep(&m_context);
}

TEST(cmpRegister, T1RnSmallerThanRm)
{
    emitInstruction16("0100001010mmmnnn", R1, R0);
    setExpectedAPSRflags("Nzcv");
    setRegisterValue(R1, 1);
    pinkySimStep(&m_context);
}

TEST(cmpRegister, T1ForceNegativeOverflow)
{
    emitInstruction16("0100001010mmmnnn", R1, R2);
    setExpectedAPSRflags("nzCV");
    setRegisterValue(R2, 0x80000000U);
    setRegisterValue(R1, 1U);
    pinkySimStep(&m_context);
}

TEST(cmpRegister, T1ForcePositiveOverflow)
{
    emitInstruction16("0100001010mmmnnn", R1, R2);
    setExpectedAPSRflags("NzcV");
    setRegisterValue(R2, 0x7FFFFFFFU);
    setRegisterValue(R1, -1U);
    pinkySimStep(&m_context);
}



/* CMP - Register - Encoding T2
   Encoding: 010001 01 N:1 Rm:4 Rn:3
   NOTE: At least one register must be high register, R8 - R14. */
TEST(cmpRegister, T2CompareLowestRegisterToHighestRegister)
{
    emitInstruction16("01000101nmmmmnnn", R0, LR);
    setRegisterValue(LR, 0xEEEEEEEE);
    setExpectedAPSRflags("nzcv");
    pinkySimStep(&m_context);
}

TEST(cmpRegister, T2CompareHighestRegisterToLowestRegister)
{
    emitInstruction16("01000101nmmmmnnn", LR, R0);
    setRegisterValue(LR, 0xEEEEEEEE);
    setExpectedAPSRflags("NzCv");
    pinkySimStep(&m_context);
}

TEST(cmpRegister, T2CompareR8ToItself)
{
    emitInstruction16("01000101nmmmmnnn", R8, R8);
    setExpectedAPSRflags("nZCv");
    pinkySimStep(&m_context);
}

TEST(cmpRegister, T2ForceNegativeOverflow)
{
    emitInstruction16("01000101nmmmmnnn", R11, R12);
    setExpectedAPSRflags("nzCV");
    setRegisterValue(R11, 0x80000000U);
    setRegisterValue(R12, 1U);
    pinkySimStep(&m_context);
}

TEST(cmpRegister, T2ForcePositiveOverflow)
{
    emitInstruction16("01000101nmmmmnnn", R11, R12);
    setExpectedAPSRflags("NzcV");
    setRegisterValue(R11, 0x7FFFFFFFU);
    setRegisterValue(R12, -1U);
    pinkySimStep(&m_context);
}

TEST(cmpRegister, T2UnpredictableForBothArgsToBeLowRegisters)
{
    emitInstruction16("01000101nmmmmnnn", R6, R7);
    setExpectedStepReturn(PINKYSIM_STEP_UNPREDICTABLE);
    pinkySimStep(&m_context);
}

TEST(cmpRegister, T2UnpredictableForRnToBeR15)
{
    emitInstruction16("01000101nmmmmnnn", PC, R8);
    setExpectedStepReturn(PINKYSIM_STEP_UNPREDICTABLE);
    pinkySimStep(&m_context);
}

TEST(cmpRegister, T2UnpredictableForRmToBeR15)
{
    emitInstruction16("01000101nmmmmnnn", R8, PC);
    setExpectedStepReturn(PINKYSIM_STEP_UNPREDICTABLE);
    pinkySimStep(&m_context);
}

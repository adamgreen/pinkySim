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

TEST_GROUP_BASE(addRegister, pinkySimBase)
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


/* ADD - Register - Encoding T1
   Encoding: 000 11 0 0 Rm:3 Rn:3 Rd:3 */
TEST(addRegister, T1UseR1ForAllArgsAndCarryOverflowZeroNegativeFlagsShouldBeClear)
{
    emitInstruction16("0001100mmmnnnddd", R1, R1, R1);
    setExpectedAPSRflags("nzcv");
    setExpectedRegisterValue(R1, 0x11111111U + 0x11111111U);
    pinkySimStep(&m_context);
}

TEST(addRegister, T1UseLowestRegisterForAllArgsAndOnlyZeroFlagShouldBeSet)
{
    emitInstruction16("0001100mmmnnnddd", R0, R0, R0);
    setExpectedAPSRflags("nZcv");
    setExpectedRegisterValue(R0, 0U);
    pinkySimStep(&m_context);
}

TEST(addRegister, T1UseHigestRegisterForAllArgsAndWillBeNegativeBecauseOfOverflow)
{
    emitInstruction16("0001100mmmnnnddd", R7, R7, R7);
    setExpectedAPSRflags("NzcV");
    setExpectedRegisterValue(R7, 0x77777777U + 0x77777777U);
    pinkySimStep(&m_context);
}

TEST(addRegister, T1UseDifferentRegistersForEachArg)
{
    emitInstruction16("0001100mmmnnnddd", R1, R2, R0);
    setExpectedAPSRflags("nzcv");
    setExpectedRegisterValue(R0, 0x11111111U + 0x22222222U);
    pinkySimStep(&m_context);
}

TEST(addRegister, T1ForceCarryWithNoOverflow)
{
    emitInstruction16("0001100mmmnnnddd", R1, R2, R0);
    setExpectedAPSRflags("nZCv");
    setRegisterValue(R1, -1);
    setRegisterValue(R2, 1);
    setExpectedRegisterValue(R0, -1 + 1);
    pinkySimStep(&m_context);
}

TEST(addRegister, T1ForceCarryAndOverflow)
{
    emitInstruction16("0001100mmmnnnddd", R1, R2, R0);
    setExpectedAPSRflags("nzCV");
    setRegisterValue(R1, -1);
    setRegisterValue(R2, 0x80000000U);
    setExpectedRegisterValue(R0, -1 + (int32_t)0x80000000U);
    pinkySimStep(&m_context);
}



/* ADD - Register - Encoding T2
   Encoding: 010001 00 DN:1 Rm:4 Rdn:3 
   NOTE: Shouldn't modify any of the APSR flags.*/
TEST(addRegister, T2UseR1ForAllArgs)
{
    emitInstruction16("01000100dmmmmddd", R1, R1);
    setExpectedRegisterValue(R1, 0x11111111U + 0x11111111U);
    pinkySimStep(&m_context);
}

TEST(addRegister, T2UseLowestRegisterForAllArgs)
{
    emitInstruction16("01000100dmmmmddd", R0, R0);
    setExpectedRegisterValue(R0, 0U);
    pinkySimStep(&m_context);
}

TEST(addRegister, T2UseR12ForAllArgs)
{
    emitInstruction16("01000100dmmmmddd", R12, R12);
    setExpectedRegisterValue(R12, 0xCCCCCCCCU + 0xCCCCCCCCU);
    pinkySimStep(&m_context);
}

TEST(addRegister, T2UseDifferentRegistersForEachArg)
{
    emitInstruction16("01000100dmmmmddd", R2, R1);
    setExpectedRegisterValue(R2, 0x11111111U + 0x22222222U);
    pinkySimStep(&m_context);
}

TEST(addRegister, T2ForceCarryWithNoOverflow)
{
    emitInstruction16("01000100dmmmmddd", R2, R1);
    setRegisterValue(R1, -1);
    setRegisterValue(R2, 1);
    setExpectedRegisterValue(R2, -1 + 1);
    pinkySimStep(&m_context);
}

TEST(addRegister, T2ForceCarryAndOverflow)
{
    emitInstruction16("01000100dmmmmddd", R2, R1);
    setRegisterValue(R1, -1);
    setRegisterValue(R2, 0x80000000U);
    setExpectedRegisterValue(R2, -1 + (int32_t)0x80000000U);
    pinkySimStep(&m_context);
}

TEST(addRegister, T2Add1ToSP)
{
    emitInstruction16("01000100dmmmmddd", SP, R1);
    setRegisterValue(SP, 0x10008000);
    setRegisterValue(R1, 1);
    setExpectedRegisterValue(SP, 0x10008000 + 1);
    pinkySimStep(&m_context);
}

TEST(addRegister, T2Subtract1FromSP)
{
    emitInstruction16("01000100dmmmmddd", SP, R1);
    setRegisterValue(SP, 0x10008000);
    setRegisterValue(R1, -1);
    setExpectedRegisterValue(SP, 0x10008000 - 1);
    pinkySimStep(&m_context);
}

TEST(addRegister, T2Add1ToLR)
{
    emitInstruction16("01000100dmmmmddd", LR, R1);
    setRegisterValue(LR, 2);
    setRegisterValue(R1, 1);
    setExpectedRegisterValue(LR, 2 + 1);
    pinkySimStep(&m_context);
}

TEST(addRegister, T2Add1ToPCWhichWillBeOddAndRoundedDown)
{
    emitInstruction16("01000100dmmmmddd", PC, R1);
    setRegisterValue(PC, 0x10000000);
    setRegisterValue(R1, 1);
    setExpectedRegisterValue(PC, (0x10000000 + 4 + 1) & 0xFFFFFFFE);
    pinkySimStep(&m_context);
}

TEST(addRegister, T2Add2ToPC)
{
    emitInstruction16("01000100dmmmmddd", PC, R1);
    setRegisterValue(PC, 0x10000000);
    setRegisterValue(R1, 2);
    setExpectedRegisterValue(PC, (0x10000000 + 4 + 2) & 0xFFFFFFFE);
    pinkySimStep(&m_context);
}

TEST(addRegister, T2ItIsUnpredictableToHaveBothArgsBePC)
{
    emitInstruction16("01000100dmmmmddd", PC, PC);
    setExpectedStepReturn(PINKYSIM_STEP_UNPREDICTABLE);
    pinkySimStep(&m_context);
}

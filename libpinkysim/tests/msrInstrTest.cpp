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

TEST_GROUP_BASE(msr, pinkySimBase)
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


/* MSR
   Encoding: 11110 0 1110 0 (0) Rn:4
             10 (0) 0 (1) (0) (0) (0) SYSm:8 */
TEST(msr, ToAPSR)
{
    emitInstruction32("111100111000nnnn", "10001000ssssssss", R12, SYS_APSR);
    setExpectedXPSRflags("NZCV");
    clearNegative(); clearZero(); clearCarry(); clearOverflow();
    setRegisterValue(R12, 0xFFFFFFFF);
    pinkySimStep(&m_context);
}

TEST(msr, ToIAPSR)
{
    emitInstruction32("111100111000nnnn", "10001000ssssssss", R12, SYS_IAPSR);
    setExpectedXPSRflags("NZCV");
    clearNegative(); clearZero(); clearCarry(); clearOverflow();
    setRegisterValue(R12, 0xFFFFFFFF);
    pinkySimStep(&m_context);
}

TEST(msr, ToEAPSR)
{
    emitInstruction32("111100111000nnnn", "10001000ssssssss", R12, SYS_EAPSR);
    setExpectedXPSRflags("NZCV");
    clearNegative(); clearZero(); clearCarry(); clearOverflow();
    setRegisterValue(R12, 0xFFFFFFFF);
    pinkySimStep(&m_context);
}

TEST(msr, ToXPSR)
{
    emitInstruction32("111100111000nnnn", "10001000ssssssss", R0, SYS_XPSR);
    setExpectedXPSRflags("NZCV");
    clearNegative(); clearZero(); clearCarry(); clearOverflow();
    setRegisterValue(R0, 0xFFFFFFFF);
    pinkySimStep(&m_context);
}

TEST(msr, ToIPSR)
{
    emitInstruction32("111100111000nnnn", "10001000ssssssss", R12, SYS_IPSR);
    setRegisterValue(R12, 0xFFFFFFFF);
    pinkySimStep(&m_context);
}

TEST(msr, ToEPSR)
{
    emitInstruction32("111100111000nnnn", "10001000ssssssss", R12, SYS_EPSR);
    setRegisterValue(R12, 0xFFFFFFFF);
    pinkySimStep(&m_context);
}

TEST(msr, ToIEPSR)
{
    emitInstruction32("111100111000nnnn", "10001000ssssssss", R12, SYS_IEPSR);
    setRegisterValue(R12, 0xFFFFFFFF);
    pinkySimStep(&m_context);
}

TEST(msr, ToMSP)
{
    emitInstruction32("111100111000nnnn", "10001000ssssssss", R12, SYS_MSP);
    setRegisterValue(R12, INITIAL_PC + 1024 + 2);
    setExpectedRegisterValue(SP, INITIAL_PC + 1024);
    pinkySimStep(&m_context);
}

TEST(msr, ToPSP)
{
    emitInstruction32("111100111000nnnn", "10001000ssssssss", R12, SYS_PSP);
    setRegisterValue(R12, 0xFFFFFFFF);
    pinkySimStep(&m_context);
}

TEST_SIM_ONLY(msr, PRIMASKto1)
{
    emitInstruction32("111100111000nnnn", "10001000ssssssss", R12, SYS_PRIMASK);
    setRegisterValue(R12, 0xFFFFFFFF);
    pinkySimStep(&m_context);
    CHECK_EQUAL(1, m_context.PRIMASK);
}

TEST_SIM_ONLY(msr, PRIMASKto0)
{
    emitInstruction32("111100111000nnnn", "10001000ssssssss", R12, SYS_PRIMASK);
    setRegisterValue(R12, 0xFFFFFFFE);
    pinkySimStep(&m_context);
    CHECK_EQUAL(0, m_context.PRIMASK);
}

TEST_SIM_ONLY(msr, CONTROLIgnored)
{
    emitInstruction32("111100111000nnnn", "10001000ssssssss", R12, SYS_CONTROL);
    setRegisterValue(R12, 0xFFFFFFFF);
    pinkySimStep(&m_context);
    CHECK_EQUAL(0, m_context.CONTROL);
}

TEST_SIM_ONLY(msr, R13IsUnpredictable)
{
    emitInstruction32("111100111000nnnn", "10001000ssssssss", SP, SYS_XPSR);
    setExpectedStepReturn(PINKYSIM_STEP_UNPREDICTABLE);
    setExpectedRegisterValue(PC, INITIAL_PC);
    pinkySimStep(&m_context);
}

TEST_SIM_ONLY(msr, R15IsUnpredictable)
{
    emitInstruction32("111100111000nnnn", "10001000ssssssss", PC, SYS_XPSR);
    setExpectedStepReturn(PINKYSIM_STEP_UNPREDICTABLE);
    setExpectedRegisterValue(PC, INITIAL_PC);
    pinkySimStep(&m_context);
}

TEST_SIM_ONLY(msr, UnpredictableSYSm)
{
    uint32_t predictables[] = {0, 1, 2, 3, 5, 6, 7, 8, 9, 16, 20};
    size_t   nextSkip = 0;

    for (uint32_t i = 0 ; i < 256 ; i++)
    {
        if (nextSkip < sizeof(predictables)/sizeof(predictables[0]) && i == predictables[nextSkip])
        {
            nextSkip++;
        }
        else
        {
            initContext();
            emitInstruction32("111100111000nnnn", "10001000ssssssss", R0, i);
            setExpectedStepReturn(PINKYSIM_STEP_UNPREDICTABLE);
            setExpectedRegisterValue(PC, INITIAL_PC);
            pinkySimStep(&m_context);
        }
    }
}

TEST_SIM_ONLY(msr, UnpredictableBecauseOfBit2_8)
{
    emitInstruction32("111100111000nnnn", "10001001ssssssss", R0, SYS_XPSR);
    setExpectedStepReturn(PINKYSIM_STEP_UNPREDICTABLE);
    setExpectedRegisterValue(PC, INITIAL_PC);
    pinkySimStep(&m_context);
}

TEST_SIM_ONLY(msr, UnpredictableBecauseOfBit2_9)
{
    emitInstruction32("111100111000nnnn", "10001010ssssssss", R0, SYS_XPSR);
    setExpectedStepReturn(PINKYSIM_STEP_UNPREDICTABLE);
    setExpectedRegisterValue(PC, INITIAL_PC);
    pinkySimStep(&m_context);
}

TEST_SIM_ONLY(msr, UnpredictableBecauseOfBit2_10)
{
    emitInstruction32("111100111000nnnn", "10001100ssssssss", R0, SYS_XPSR);
    setExpectedStepReturn(PINKYSIM_STEP_UNPREDICTABLE);
    setExpectedRegisterValue(PC, INITIAL_PC);
    pinkySimStep(&m_context);
}

TEST_SIM_ONLY(msr, UnpredictableBecauseOfBit2_11)
{
    emitInstruction32("111100111000nnnn", "10000000ssssssss", R0, SYS_XPSR);
    setExpectedStepReturn(PINKYSIM_STEP_UNPREDICTABLE);
    setExpectedRegisterValue(PC, INITIAL_PC);
    pinkySimStep(&m_context);
}

TEST_SIM_ONLY(msr, UnpredictableBecauseOfBit2_13)
{
    emitInstruction32("111100111000nnnn", "10101000ssssssss", R0, SYS_XPSR);
    setExpectedStepReturn(PINKYSIM_STEP_UNPREDICTABLE);
    setExpectedRegisterValue(PC, INITIAL_PC);
    pinkySimStep(&m_context);
}

TEST_SIM_ONLY(msr, UnpredictableBecauseOfBit1_4)
{
    emitInstruction32("111100111001nnnn", "10001000ssssssss", R0, SYS_XPSR);
    setExpectedStepReturn(PINKYSIM_STEP_UNPREDICTABLE);
    setExpectedRegisterValue(PC, INITIAL_PC);
    pinkySimStep(&m_context);
}

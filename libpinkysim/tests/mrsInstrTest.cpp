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

TEST_GROUP_BASE(mrs, pinkySimBase)
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

// IPSR value can't be changed from within a debug monitor.
#ifdef THUNK2REAL
    #define IPSR_VAL 0x0
#else
    #define IPSR_VAL 0x20
#endif


/* MRS
   Encoding: 11110 0 1111 1 (0) (1)(1)(1)(1)
             10 (0) 0 Rd:4 SYSm:8 */
TEST(mrs, FromAPSR)
{
    emitInstruction32("1111001111101111", "1000ddddssssssss", R12, SYS_APSR);
    setExpectedXPSRflags("NzCv");
    setNegative(); clearZero(); setCarry(); clearOverflow();
    setIPSR(IPSR_VAL);
    setExpectedIPSR(IPSR_VAL);
    setRegisterValue(R12, 0xFFFFFFFF);
    setExpectedRegisterValue(R12, APSR_N | APSR_C);
    pinkySimStep(&m_context);
}

TEST(mrs, FromIAPSR)
{
    emitInstruction32("1111001111101111", "1000ddddssssssss", R0, SYS_IAPSR);
    setExpectedXPSRflags("NzCv");
    setNegative(); clearZero(); setCarry(); clearOverflow();
    setIPSR(IPSR_VAL);
    setExpectedIPSR(IPSR_VAL);
    setRegisterValue(R0, 0xFFFFFFFF);
    setExpectedRegisterValue(R0, APSR_N | APSR_C | IPSR_VAL);
    pinkySimStep(&m_context);
}

TEST(mrs, FromEAPSR)
{
    emitInstruction32("1111001111101111", "1000ddddssssssss", R12, SYS_EAPSR);
    setExpectedXPSRflags("NzCv");
    setNegative(); clearZero(); setCarry(); clearOverflow();
    setIPSR(IPSR_VAL);
    setExpectedIPSR(IPSR_VAL);
    setRegisterValue(R12, 0xFFFFFFFF);
    setExpectedRegisterValue(R12, APSR_N | APSR_C);
    pinkySimStep(&m_context);
}

TEST(mrs, FromXPSR)
{
    emitInstruction32("1111001111101111", "1000ddddssssssss", R12, SYS_XPSR);
    setExpectedXPSRflags("NzCv");
    setNegative(); clearZero(); setCarry(); clearOverflow();
    setIPSR(IPSR_VAL);
    setExpectedIPSR(IPSR_VAL);
    setRegisterValue(R12, 0xFFFFFFFF);
    setExpectedRegisterValue(R12, APSR_N | APSR_C | IPSR_VAL);
    pinkySimStep(&m_context);
}

TEST(mrs, FromIPSR)
{
    emitInstruction32("1111001111101111", "1000ddddssssssss", R12, SYS_IPSR);
    setExpectedXPSRflags("NzCv");
    setNegative(); clearZero(); setCarry(); clearOverflow();
    setIPSR(IPSR_VAL);
    setExpectedIPSR(IPSR_VAL);
    setRegisterValue(R12, 0xFFFFFFFF);
    setExpectedRegisterValue(R12, IPSR_VAL);
    pinkySimStep(&m_context);
}

TEST(mrs, FromEPSR)
{
    emitInstruction32("1111001111101111", "1000ddddssssssss", R12, SYS_EPSR);
    setExpectedXPSRflags("NzCv");
    setNegative(); clearZero(); setCarry(); clearOverflow();
    setIPSR(IPSR_VAL);
    setExpectedIPSR(IPSR_VAL);
    setRegisterValue(R12, 0xFFFFFFFF);
    setExpectedRegisterValue(R12, 0);
    pinkySimStep(&m_context);
}

TEST(mrs, FromIEPSR)
{
    emitInstruction32("1111001111101111", "1000ddddssssssss", R12, SYS_IEPSR);
    setExpectedXPSRflags("NzCv");
    setNegative(); clearZero(); setCarry(); clearOverflow();
    setIPSR(IPSR_VAL);
    setExpectedIPSR(IPSR_VAL);
    setRegisterValue(R12, 0xFFFFFFFF);
    setExpectedRegisterValue(R12, IPSR_VAL);
    pinkySimStep(&m_context);
}

TEST(mrs, FromMSP)
{
    emitInstruction32("1111001111101111", "1000ddddssssssss", R12, SYS_MSP);
    setRegisterValue(R12, 0xFFFFFFFF);
    setExpectedRegisterValue(R12, INITIAL_SP);
    pinkySimStep(&m_context);
}

TEST_SIM_ONLY(mrs, FromPSP)
{
    emitInstruction32("1111001111101111", "1000ddddssssssss", R12, SYS_PSP);
    setRegisterValue(R12, 0xFFFFFFFF);
    setExpectedRegisterValue(R12, 0x0);
    pinkySimStep(&m_context);
}

TEST_SIM_ONLY(mrs, FromPRIMASKsetTo1)
{
    emitInstruction32("1111001111101111", "1000ddddssssssss", R12, SYS_PRIMASK);
    setRegisterValue(R12, 0xFFFFFFFF);
    m_context.PRIMASK = 1;
    setExpectedRegisterValue(R12, 1);
    pinkySimStep(&m_context);
}

TEST_SIM_ONLY(mrs, PRIMASKto0)
{
    emitInstruction32("1111001111101111", "1000ddddssssssss", R12, SYS_PRIMASK);
    setRegisterValue(R12, 0xFFFFFFFF);
    m_context.PRIMASK = 0;
    setExpectedRegisterValue(R12, 0);
    pinkySimStep(&m_context);
}

TEST_SIM_ONLY(mrs, CONTROLIgnored)
{
    emitInstruction32("1111001111101111", "1000ddddssssssss", R12, SYS_CONTROL);
    setRegisterValue(R12, 0xFFFFFFFF);
    setExpectedRegisterValue(R12, 0);
    pinkySimStep(&m_context);
}

TEST_SIM_ONLY(mrs, R13IsUnpredictable)
{
    emitInstruction32("1111001111101111", "1000ddddssssssss", SP, SYS_XPSR);
    setExpectedStepReturn(PINKYSIM_STEP_UNPREDICTABLE);
    setExpectedRegisterValue(PC, INITIAL_PC);
    pinkySimStep(&m_context);
}

TEST_SIM_ONLY(mrs, R15IsUnpredictable)
{
    emitInstruction32("1111001111101111", "1000ddddssssssss", PC, SYS_XPSR);
    setExpectedStepReturn(PINKYSIM_STEP_UNPREDICTABLE);
    setExpectedRegisterValue(PC, INITIAL_PC);
    pinkySimStep(&m_context);
}

TEST_SIM_ONLY(mrs, UnpredictableSYSm)
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
            emitInstruction32("1111001111101111", "1000ddddssssssss", R0, i);
            setRegisterValue(R0, 0xFFFFFFFF);
            setExpectedStepReturn(PINKYSIM_STEP_UNPREDICTABLE);
            setExpectedRegisterValue(PC, INITIAL_PC);
            pinkySimStep(&m_context);
        }
    }
}

TEST_SIM_ONLY(mrs, UnpredictableBecauseOfBit2_13)
{
    emitInstruction32("1111001111101111", "1010ddddssssssss", R12, SYS_XPSR);
    setRegisterValue(R0, 0xFFFFFFFF);
    setExpectedStepReturn(PINKYSIM_STEP_UNPREDICTABLE);
    setExpectedRegisterValue(PC, INITIAL_PC);
    pinkySimStep(&m_context);
}

TEST_SIM_ONLY(mrs, UnpredictableBecauseOfBit1_0)
{
    emitInstruction32("1111001111101110", "1000ddddssssssss", R12, SYS_XPSR);
    setRegisterValue(R0, 0xFFFFFFFF);
    setExpectedStepReturn(PINKYSIM_STEP_UNPREDICTABLE);
    setExpectedRegisterValue(PC, INITIAL_PC);
    pinkySimStep(&m_context);
}

TEST_SIM_ONLY(mrs, UnpredictableBecauseOfBit1_1)
{
    emitInstruction32("1111001111101101", "1000ddddssssssss", R12, SYS_XPSR);
    setRegisterValue(R0, 0xFFFFFFFF);
    setExpectedStepReturn(PINKYSIM_STEP_UNPREDICTABLE);
    setExpectedRegisterValue(PC, INITIAL_PC);
    pinkySimStep(&m_context);
}

TEST_SIM_ONLY(mrs, UnpredictableBecauseOfBit1_2)
{
    emitInstruction32("1111001111101011", "1000ddddssssssss", R12, SYS_XPSR);
    setRegisterValue(R0, 0xFFFFFFFF);
    setExpectedStepReturn(PINKYSIM_STEP_UNPREDICTABLE);
    setExpectedRegisterValue(PC, INITIAL_PC);
    pinkySimStep(&m_context);
}

TEST_SIM_ONLY(mrs, UnpredictableBecauseOfBit1_3)
{
    emitInstruction32("1111001111100111", "1000ddddssssssss", R12, SYS_XPSR);
    setRegisterValue(R0, 0xFFFFFFFF);
    setExpectedStepReturn(PINKYSIM_STEP_UNPREDICTABLE);
    setExpectedRegisterValue(PC, INITIAL_PC);
    pinkySimStep(&m_context);
}

TEST_SIM_ONLY(mrs, UnpredictableBecauseOfBit1_4)
{
    emitInstruction32("1111001111111111", "1000ddddssssssss", R12, SYS_XPSR);
    setRegisterValue(R0, 0xFFFFFFFF);
    setExpectedStepReturn(PINKYSIM_STEP_UNPREDICTABLE);
    setExpectedRegisterValue(PC, INITIAL_PC);
    pinkySimStep(&m_context);
}

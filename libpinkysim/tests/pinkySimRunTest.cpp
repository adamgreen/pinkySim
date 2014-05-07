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

TEST_GROUP_BASE(pinkySimRun, pinkySimBase)
{
    void setup()
    {
        pinkySimBase::setup();
    }

    void teardown()
    {
        pinkySimBase::teardown();
    }

    void emitBKPT(uint32_t immediate)
    {
        emitInstruction16("10111110iiiiiiii", immediate);
    }

    void emitNOP()
    {
        emitInstruction16("1011111100000000");
    }

    void emitSVC(uint32_t immediate)
    {
        emitInstruction16("11011111iiiiiiii", immediate);
    }

    void emitLDRImmediate(uint32_t Rt, uint32_t Rn, uint32_t immediate)
    {
        emitInstruction16("01101iiiiinnnttt", immediate, Rn, Rt);
    }

    void emitUND(uint32_t immediate)
    {
        emitInstruction16("11011110iiiiiiii", immediate);
    }

    void emitYIELD()
    {
        emitInstruction16("1011111100010000");
    }
};


TEST(pinkySimRun, ShouldStopImmediatelyOnBreakpoint)
{
    emitBKPT(0);
    setExpectedRegisterValue(PC, INITIAL_PC);
        int result = pinkySimRun(&m_context, NULL);
    CHECK_EQUAL(PINKYSIM_STEP_BKPT, result);
    validateXPSR();
    validateRegisters();
}

TEST(pinkySimRun, ExecuteNOPAndThenStopAtBreakpoint)
{
    emitNOP();
    emitBKPT(0);
    setExpectedRegisterValue(PC, INITIAL_PC + 2);
        int result = pinkySimRun(&m_context, NULL);
    CHECK_EQUAL(PINKYSIM_STEP_BKPT, result);
    validateXPSR();
    validateRegisters();
}

TEST(pinkySimRun, ShouldAdvanceAndStopOnSVC)
{
    emitSVC(0);
    setExpectedRegisterValue(PC, INITIAL_PC + 2);
        int result = pinkySimRun(&m_context, NULL);
    CHECK_EQUAL(PINKYSIM_STEP_SVC, result);
    validateXPSR();
    validateRegisters();
}

TEST(pinkySimRun, ShouldStopImmediatelyOnUnalignedHardfault)
{
    emitLDRImmediate(R2, R3, 0);
    setRegisterValue(R3, INITIAL_PC + 2);
    setExpectedRegisterValue(PC, INITIAL_PC);
        int result = pinkySimRun(&m_context, NULL);
    CHECK_EQUAL(PINKYSIM_STEP_HARDFAULT, result);
    validateXPSR();
    validateRegisters();
}

TEST(pinkySimRun, ShouldStopImmediatelyOnUndefinedInstruction)
{
    emitUND(0);
    setExpectedRegisterValue(PC, INITIAL_PC);
        int result = pinkySimRun(&m_context, NULL);
    CHECK_EQUAL(PINKYSIM_STEP_UNDEFINED, result);
    validateXPSR();
    validateRegisters();
}

TEST(pinkySimRun, ShouldStopImmediatelyOnUnpredictableInstructionEncoding)
{
    emitInstruction16("0100010100000000");
    setExpectedRegisterValue(PC, INITIAL_PC);
        int result = pinkySimRun(&m_context, NULL);
    CHECK_EQUAL(PINKYSIM_STEP_UNPREDICTABLE, result);
    validateXPSR();
    validateRegisters();
}

TEST(pinkySimRun, ShouldAdvanceAndThenStopForUnsupportedInstruction)
{
    emitYIELD();
    setExpectedRegisterValue(PC, INITIAL_PC + 2);
        int result = pinkySimRun(&m_context, NULL);
    CHECK_EQUAL(PINKYSIM_STEP_UNSUPPORTED, result);
    validateXPSR();
    validateRegisters();
}

TEST(pinkySimRun, HaveCallbackReturnOkAndStopAtSecondBreakpoint)
{
    emitNOP();
    emitBKPT(0);
    setExpectedRegisterValue(PC, INITIAL_PC + 2);
        int result = pinkySimRun(&m_context, NULL);
    CHECK_EQUAL(PINKYSIM_STEP_BKPT, result);
    validateXPSR();
    validateRegisters();
}

int interruptCallback(PinkySimContext* pContext)
{
    return PINKYSIM_RUN_INTERRUPT;
}

TEST(pinkySimRun, HaveCallbackReturnInterruptResponseToStopImmediately)
{
    emitNOP();
    setExpectedRegisterValue(PC, INITIAL_PC);
        int result = pinkySimRun(&m_context, interruptCallback);
    CHECK_EQUAL(PINKYSIM_RUN_INTERRUPT, result);
    validateXPSR();
    validateRegisters();
}

int watchpointCallback(PinkySimContext* pContext)
{
    return PINKYSIM_RUN_WATCHPOINT;
}

TEST(pinkySimRun, HaveCallbackReturnWatchpointResponseToStopImmediately)
{
    emitNOP();
    setExpectedRegisterValue(PC, INITIAL_PC);
        int result = pinkySimRun(&m_context, watchpointCallback);
    CHECK_EQUAL(PINKYSIM_RUN_WATCHPOINT, result);
    validateXPSR();
    validateRegisters();
}

int nullCallback(PinkySimContext* pContext)
{
    return PINKYSIM_STEP_OK;
}

TEST(pinkySimRun, HaveCallbackReturnOkToContinueToSVC)
{
    emitNOP();
    emitSVC(0);
    setExpectedRegisterValue(PC, INITIAL_PC + 4);
        int result = pinkySimRun(&m_context, nullCallback);
    CHECK_EQUAL(PINKYSIM_STEP_SVC, result);
    validateXPSR();
    validateRegisters();
}

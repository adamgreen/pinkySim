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
#include <signal.h>
#include "mri4simBaseTest.h"

TEST_GROUP_BASE(stepTests, mri4simBase)
{
    void setup()
    {
        mri4simBase::setup();
    }

    void teardown()
    {
        mri4simBase::teardown();
    }
};


TEST(stepTests, StepOverNOP)
{
    emitNOP();
    emitNOP();

    mockIComm_InitReceiveChecksummedData("+$s#");
        mri4simRun(mockIComm_Get(), TRUE);
    appendExpectedTPacket(SIGTRAP, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC);
    appendExpectedString("+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());

    mockIComm_InitTransmitDataBuffer(1024);
    mockIComm_InitReceiveChecksummedData("+$c#");
    mockIComm_DelayReceiveData(1);
        mri4simRun(mockIComm_Get(), FALSE);
    resetExpectedBuffer();
    appendExpectedTPacket(SIGTRAP, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC + 2);
    appendExpectedString("+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
    CHECK_EQUAL(INITIAL_PC + 2, m_pContext->pc);
}

TEST(stepTests, StepOverBKPT_JustAdvancesPCAndSendsTPacket)
{
    emitBKPT(0);
    emitNOP();

    mockIComm_InitReceiveChecksummedData("+$s#", "+$c#");
        mri4simRun(mockIComm_Get(), TRUE);
    appendExpectedTPacket(SIGTRAP, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC);
    appendExpectedString("+");
    appendExpectedTPacket(SIGTRAP, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC + 2);
    appendExpectedString("+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());

}

TEST(stepTests, BreakOnStart_SendStepWithSpecificAddressToSkipSomeInstructions)
{
    emitNOP();
    emitNOP();
    emitNOP();

    char commands[64];
    snprintf(commands, sizeof(commands), "+$s%x#", INITIAL_PC + 4);
    mockIComm_InitReceiveChecksummedData(commands);
        mri4simRun(mockIComm_Get(), TRUE);
    appendExpectedTPacket(SIGTRAP, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC);
    appendExpectedString("+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
    CHECK_EQUAL(INITIAL_PC + 4, m_pContext->pc);

    mockIComm_InitTransmitDataBuffer(1024);
    mockIComm_InitReceiveChecksummedData("+$c#");
    mockIComm_DelayReceiveData(1);
        mri4simRun(mockIComm_Get(), FALSE);
    resetExpectedBuffer();
    appendExpectedTPacket(SIGTRAP, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC + 6);
    appendExpectedString("+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
    CHECK_EQUAL(INITIAL_PC + 6, m_pContext->pc);
}

TEST(stepTests, StepOverNOPAndThenContinueToBKPT)
{
    emitNOP();
    emitNOP();
    emitNOP();
    emitBKPT(0);

    mockIComm_InitReceiveChecksummedData("+$s#");
        mri4simRun(mockIComm_Get(), TRUE);
    appendExpectedTPacket(SIGTRAP, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC);
    appendExpectedString("+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());

    mockIComm_InitTransmitDataBuffer(1024);
    mockIComm_InitReceiveChecksummedData("+$c#");
    mockIComm_DelayReceiveData(1);
        mri4simRun(mockIComm_Get(), FALSE);
    resetExpectedBuffer();
    appendExpectedTPacket(SIGTRAP, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC + 2);
    appendExpectedString("+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());

    mockIComm_InitTransmitDataBuffer(1024);
    mockIComm_InitReceiveChecksummedData("+$c#");
    mockIComm_DelayReceiveData(3);
        mri4simRun(mockIComm_Get(), FALSE);
    resetExpectedBuffer();
    appendExpectedTPacket(SIGTRAP, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC + 6);
    appendExpectedString("+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
}

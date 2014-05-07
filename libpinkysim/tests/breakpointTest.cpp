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
extern "C"
{
    #include <signal.h>
    #include <mri.h>
    #include <MallocFailureInject.h>
}
#include "mri4simBaseTest.h"

TEST_GROUP_BASE(breakpointTests, mri4simBase)
{
    void setup()
    {
        mri4simBase::setup();
    }

    void teardown()
    {
        mri4simBase::teardown();
        MallocFailureInject_Restore();
    }
};


TEST(breakpointTests, Set16bitBreakpointAndRunUntilHit)
{
    emitNOP();
    emitNOP();
    emitNOP();
    emitNOP();
    emitBKPT(0);

    char commands[64];
    snprintf(commands, sizeof(commands), "+$Z1,%x,2#", INITIAL_PC + 6);
    mockIComm_InitReceiveChecksummedData(commands, "+$c#");
        mri4simRun(mockIComm_Get(), TRUE);
    appendExpectedTPacket(SIGTRAP, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC);
    appendExpectedString("+$OK#+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());

    mockIComm_InitTransmitDataBuffer(1024);
    mockIComm_InitReceiveChecksummedData("+$c#");
    mockIComm_DelayReceiveData(4);
        mri4simRun(mockIComm_Get(), FALSE);
    resetExpectedBuffer();
    appendExpectedTPacket(SIGTRAP, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC + 6);
    appendExpectedString("+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
    CHECK_EQUAL(INITIAL_PC + 6, m_pContext->pc);
}

TEST(breakpointTests, Set32bitBreakpointAndRunUntilHit)
{
    emitNOP();
    emitNOP();
    emitNOP();
    /* This is a 32-bit BL instruction. */
    emitInstruction32("11110Siiiiiiiiii", "11j1kiiiiiiiiiii", 0, 0, 1, 1, 0);
    emitBKPT(0);

    char commands[64];
    snprintf(commands, sizeof(commands), "+$Z1,%x,3#", INITIAL_PC + 6);
    mockIComm_InitReceiveChecksummedData(commands, "+$c#");
        mri4simRun(mockIComm_Get(), TRUE);
    appendExpectedTPacket(SIGTRAP, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC);
    appendExpectedString("+$OK#+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());

    mockIComm_InitTransmitDataBuffer(1024);
    mockIComm_InitReceiveChecksummedData("+$c#");
    mockIComm_DelayReceiveData(4);
        mri4simRun(mockIComm_Get(), FALSE);
    resetExpectedBuffer();
    appendExpectedTPacket(SIGTRAP, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC + 6);
    appendExpectedString("+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
    CHECK_EQUAL(INITIAL_PC + 6, m_pContext->pc);
}

TEST(breakpointTests, AttemptToSetInvalidBreakpoint_ShouldReturnErrorMessage)
{
    emitBKPT(0);

    char commands[64];
    snprintf(commands, sizeof(commands), "+$Z1,%x,1#", INITIAL_PC);
    mockIComm_InitReceiveChecksummedData(commands, "+$c#");
        mri4simRun(mockIComm_Get(), TRUE);
    appendExpectedTPacket(SIGTRAP, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC);
    appendExpectedString("+$" MRI_ERROR_INVALID_ARGUMENT "#+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
}

TEST(breakpointTests, FailMemoryAllocationOnBreakpointSet_ShouldReturnErrorMessage)
{
    emitNOP();
    emitNOP();
    emitNOP();
    emitNOP();
    emitBKPT(0);

    char commands[64];
    snprintf(commands, sizeof(commands), "+$Z1,%x,2#", INITIAL_PC + 6);
    mockIComm_InitReceiveChecksummedData(commands, "+$c#");
    MallocFailureInject_FailAllocation(1);
        mri4simRun(mockIComm_Get(), TRUE);
    appendExpectedTPacket(SIGTRAP, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC);
    appendExpectedString("+$" MRI_ERROR_NO_FREE_BREAKPOINT "#+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
}

TEST(breakpointTests, AttemptToClearInvalidBreakpointType_ShouldReturnErrorMessage)
{
    emitBKPT(0);

    char commands[64];
    snprintf(commands, sizeof(commands), "+$z1,%x,1#", INITIAL_PC);
    mockIComm_InitReceiveChecksummedData(commands, "+$c#");
        mri4simRun(mockIComm_Get(), TRUE);
    appendExpectedTPacket(SIGTRAP, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC);
    appendExpectedString("+$" MRI_ERROR_INVALID_ARGUMENT "#+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
}

TEST(breakpointTests, AttemptToSetBreakpointAtInvalidAddress_ShouldReturnErrorMessage)
{
    emitBKPT(0);

    mockIComm_InitReceiveChecksummedData("+$Z1,fffffffe,2#", "+$c#");
        mri4simRun(mockIComm_Get(), TRUE);
    appendExpectedTPacket(SIGTRAP, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC);
    appendExpectedString("+$" MRI_ERROR_NO_FREE_BREAKPOINT "#+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
}

TEST(breakpointTests, AttemptToClearBreakpointAtInvalidAddress_ShouldReturnErrorMessage)
{
    emitBKPT(0);

    mockIComm_InitReceiveChecksummedData("+$z1,fffffffe,2#", "+$c#");
        mri4simRun(mockIComm_Get(), TRUE);
    appendExpectedTPacket(SIGTRAP, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC);
    appendExpectedString("+$" MRI_ERROR_INVALID_ARGUMENT "#+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
}

TEST(breakpointTests, SetBreakpointHit_LeaveBreakpointSet_ContinueAndHitAgain)
{
    emitNOP();
    emitNOP();
    emitNOP();
    emitNOP();
    emitBKPT(0);

    char commands[64];
    snprintf(commands, sizeof(commands), "+$Z1,%x,2#", INITIAL_PC + 4);
    mockIComm_InitReceiveChecksummedData(commands, "+$c#");
        mri4simRun(mockIComm_Get(), TRUE);
    appendExpectedTPacket(SIGTRAP, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC);
    appendExpectedString("+$OK#+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());

    mockIComm_InitTransmitDataBuffer(1024);
    mockIComm_InitReceiveChecksummedData("+$c#");
    mockIComm_DelayReceiveData(3);
        mri4simRun(mockIComm_Get(), FALSE);
    resetExpectedBuffer();
    appendExpectedTPacket(SIGTRAP, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC + 4);
    appendExpectedString("+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
    CHECK_EQUAL(INITIAL_PC + 4, m_pContext->pc);

    mockIComm_InitTransmitDataBuffer(1024);
    mockIComm_InitReceiveChecksummedData("+$c#");
    mockIComm_DelayReceiveData(3);
        mri4simRun(mockIComm_Get(), FALSE);
    resetExpectedBuffer();
    appendExpectedTPacket(SIGTRAP, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC + 4);
    appendExpectedString("+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
}

TEST(breakpointTests, SetBreakpointHit_ClearBreakpoint_ContinueAndRunPast)
{
    emitNOP();
    emitNOP();
    emitNOP();
    emitNOP();
    emitBKPT(0);

    char commands[64];
    snprintf(commands, sizeof(commands), "+$Z1,%x,2#", INITIAL_PC + 4);
    mockIComm_InitReceiveChecksummedData(commands, "+$c#");
        mri4simRun(mockIComm_Get(), TRUE);
    appendExpectedTPacket(SIGTRAP, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC);
    appendExpectedString("+$OK#+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());

    mockIComm_InitTransmitDataBuffer(1024);
    snprintf(commands, sizeof(commands), "+$z1,%x,2#", INITIAL_PC + 4);
    mockIComm_InitReceiveChecksummedData(commands, "+$c#");
    mockIComm_DelayReceiveData(3);
        mri4simRun(mockIComm_Get(), FALSE);
    resetExpectedBuffer();
    appendExpectedTPacket(SIGTRAP, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC + 4);
    appendExpectedString("+$OK#+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
    CHECK_EQUAL(INITIAL_PC + 4, m_pContext->pc);

    mockIComm_InitTransmitDataBuffer(1024);
    mockIComm_InitReceiveChecksummedData("+$c#");
    mockIComm_DelayReceiveData(3);
        mri4simRun(mockIComm_Get(), FALSE);
    resetExpectedBuffer();
    appendExpectedTPacket(SIGTRAP, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC + 8);
    appendExpectedString("+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
}

TEST(breakpointTests, Set32bitBreakpoint_RunUntilHit_Clear32bitBreakpoint)
{
    emitNOP();
    emitNOP();
    emitNOP();
    /* This is a 32-bit BL instruction. */
    emitInstruction32("11110Siiiiiiiiii", "11j1kiiiiiiiiiii", 0, 0, 1, 1, 0);
    emitBKPT(0);

    char commands[64];
    snprintf(commands, sizeof(commands), "+$Z1,%x,3#", INITIAL_PC + 6);
    mockIComm_InitReceiveChecksummedData(commands, "+$c#");
        mri4simRun(mockIComm_Get(), TRUE);
    appendExpectedTPacket(SIGTRAP, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC);
    appendExpectedString("+$OK#+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());

    mockIComm_InitTransmitDataBuffer(1024);
    snprintf(commands, sizeof(commands), "+$z1,%x,3#", INITIAL_PC + 6);
    mockIComm_InitReceiveChecksummedData(commands, "+$c#");
    mockIComm_DelayReceiveData(4);
        mri4simRun(mockIComm_Get(), FALSE);
    resetExpectedBuffer();
    appendExpectedTPacket(SIGTRAP, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC + 6);
    appendExpectedString("+$OK#+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
    CHECK_EQUAL(INITIAL_PC + 6, m_pContext->pc);
}

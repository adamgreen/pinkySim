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
#include <NewlibSemihost.h>
#include "mri4simBaseTest.h"

TEST_GROUP_BASE(mri4simRun, mri4simBase)
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


TEST(mri4simRun, QueueUpDebuggerContinueCommand_ShouldInterruptSimAndGoStraightToMri)
{
    mockIComm_InitReceiveChecksummedData("+$c#");
        mri4simRun(mockIComm_Get(), FALSE);
    appendExpectedTPacket(SIGINT, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC);
    appendExpectedString("+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
    CHECK_EQUAL(INITIAL_PC, m_pContext->pc);
}

TEST(mri4simRun, DontInterruptSimAndLetRunToBreakpoint_SendContinue_ShouldAdvancePastBreakpoint)
{
    emitNOP();
    emitBKPT(0);
    mockIComm_InitReceiveChecksummedData("+$c#");
    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get(), FALSE);
    appendExpectedTPacket(SIGTRAP, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC + 2);
    appendExpectedString("+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
    CHECK_EQUAL(INITIAL_PC + 4, m_pContext->pc);
}

TEST(mri4simRun, BreakOnStart_SendContinue)
{
    emitNOP();
    emitBKPT(0);
    mockIComm_InitReceiveChecksummedData("+$c#");
    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get(), TRUE);
    appendExpectedTPacket(SIGTRAP, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC);
    appendExpectedString("+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
    CHECK_EQUAL(INITIAL_PC, m_pContext->pc);
}

TEST(mri4simRun, BreakOnStart_SendContinueWithSpecificAddressToSkipSomeInstructions)
{
    emitNOP();
    emitNOP();
    emitBKPT(0);
    char commands[64];
    snprintf(commands, sizeof(commands), "+$c%x#", INITIAL_PC + 4);
    mockIComm_InitReceiveChecksummedData(commands);
        mri4simRun(mockIComm_Get(), TRUE);
    appendExpectedTPacket(SIGTRAP, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC);
    appendExpectedString("+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
    CHECK_EQUAL(INITIAL_PC + 4, m_pContext->pc);
}

TEST(mri4simRun, IssueExitSemihostCall_ShouldExitRunLoopImmediately_NotEnterDebugger)
{
    emitBKPT(NEWLIB_EXIT);
        mri4simRun(mockIComm_Get(), FALSE);
    STRCMP_EQUAL("", mockIComm_GetTransmittedData());
    CHECK_EQUAL(INITIAL_PC, m_pContext->pc);
}

TEST(mri4simRun, UnalignedAccess_WillHardfault_ShouldStopImmediately_DumpHardFaultMessage)
{
    const char* expectedMessage = "\n**Hard Fault**\n";
    emitLDRImmediate(R2, R3, 0);
    setRegisterValue(R3, INITIAL_PC + 2);
    mockIComm_InitReceiveChecksummedData("++$c#");
    mockIComm_DelayReceiveData(1);
        mri4simRun(mockIComm_Get(), FALSE);
    appendExpectedOPacket(expectedMessage);
    appendExpectedTPacket(SIGSEGV, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC);
    appendExpectedString("+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
    STRCMP_EQUAL(expectedMessage, printfSpy_GetLastOutput());
    CHECK_EQUAL(INITIAL_PC, m_pContext->pc);
}

TEST(mri4simRun, InvalidMemoryAddress_WillHardfault_ShouldStopImmediately_DumpHardFaultMessage)
{
    const char* expectedMessage = "\n**Hard Fault**\n";
    emitLDRImmediate(R2, R3, 0);
    setRegisterValue(R3, 0xFFFFFFFC);
    mockIComm_InitReceiveChecksummedData("++$c#");
    mockIComm_DelayReceiveData(1);
        mri4simRun(mockIComm_Get(), FALSE);
    appendExpectedOPacket(expectedMessage);
    appendExpectedTPacket(SIGSEGV, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC);
    appendExpectedString("+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
    STRCMP_EQUAL(expectedMessage, printfSpy_GetLastOutput());
    CHECK_EQUAL(INITIAL_PC, m_pContext->pc);
}

TEST(mri4simRun, UndefinedInstruction_ShouldStopImmediately_DumpUndefinedMessage)
{
    const char* expectedMessage = "\n**Undefined Instruction**\n";
    emitUND(0);
    mockIComm_InitReceiveChecksummedData("++$c#");
    mockIComm_DelayReceiveData(1);
        mri4simRun(mockIComm_Get(), FALSE);
    appendExpectedOPacket(expectedMessage);
    appendExpectedTPacket(SIGILL, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC);
    appendExpectedString("+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
    STRCMP_EQUAL(expectedMessage, printfSpy_GetLastOutput());
    CHECK_EQUAL(INITIAL_PC, m_pContext->pc);
}

TEST(mri4simRun, UnpredictableInstruction_ShouldStopImmediately_DumpUnpredictableMessage)
{
    const char* expectedMessage = "\n**Unpredictable Instruction Encoding**\n";
    emitInstruction16("0100010100000000");
    mockIComm_InitReceiveChecksummedData("++$c#");
    mockIComm_DelayReceiveData(1);
        mri4simRun(mockIComm_Get(), FALSE);
    appendExpectedOPacket(expectedMessage);
    appendExpectedTPacket(SIGILL, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC);
    appendExpectedString("+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
    STRCMP_EQUAL(expectedMessage, printfSpy_GetLastOutput());
    CHECK_EQUAL(INITIAL_PC, m_pContext->pc);
}

TEST(mri4simRun, UnsupportedInstruction_ShouldStopAfter_DumpUnsupportedMessage)
{
    const char* expectedMessage = "\n**Unsupported Instruction**\n";
    emitYIELD();
    mockIComm_InitReceiveChecksummedData("++$c#");
    mockIComm_DelayReceiveData(1);
        mri4simRun(mockIComm_Get(), FALSE);
    appendExpectedOPacket(expectedMessage);
    appendExpectedTPacket(SIGILL, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC + 2);
    appendExpectedString("+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
    STRCMP_EQUAL(expectedMessage, printfSpy_GetLastOutput());
    CHECK_EQUAL(INITIAL_PC + 2, m_pContext->pc);
}

TEST(mri4simRun, UnsupportedSVCInstruction_ShouldStopAfter_DumpUnsupportedMessage)
{
    const char* expectedMessage = "\n**Unsupported Instruction**\n";
    emitSVC(0);
    mockIComm_InitReceiveChecksummedData("++$c#");
    mockIComm_DelayReceiveData(1);
        mri4simRun(mockIComm_Get(), FALSE);
    appendExpectedOPacket(expectedMessage);
    appendExpectedTPacket(SIGILL, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC + 2);
    appendExpectedString("+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
    STRCMP_EQUAL(expectedMessage, printfSpy_GetLastOutput());
    CHECK_EQUAL(INITIAL_PC + 2, m_pContext->pc);
}

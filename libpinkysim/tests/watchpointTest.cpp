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
    #include <MallocFailureInject.h>
    #include <mri.h>
    #include <signal.h>
}
#include "mri4simBaseTest.h"

TEST_GROUP_BASE(watchpointTests, mri4simBase)
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


TEST(watchpointTests, Set4ByteReadWatchpoint_RunUntilThatAddressIsRead_ButHaltsOnNextInstruction)
{
    emitNOP();
    emitNOP();
    emitLDRImmediate(R0, R1, 0);
    emitNOP();
    emitNOP();
    emitBKPT(0);
    
    char commands[64];
    snprintf(commands, sizeof(commands), "+$Z3,%x,4#", INITIAL_SP - 4);
    mockIComm_InitReceiveChecksummedData(commands, "+$c#");
    m_pContext->R[1] = INITIAL_SP - 4;
    IMemory_Write32(m_pContext->pMemory, INITIAL_SP - 4, 0xBAADF00D);
        mri4simRun(mockIComm_Get(), TRUE);
    appendExpectedTPacket(SIGTRAP, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC);
    appendExpectedString("+$OK#+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());

    mockIComm_InitTransmitDataBuffer(1024);
    mockIComm_InitReceiveChecksummedData("+$c#");
    mockIComm_DelayReceiveData(6);
        mri4simRun(mockIComm_Get(), FALSE);
    resetExpectedBuffer();
    appendExpectedTPacket(SIGTRAP, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC + 6);
    appendExpectedString("+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
    CHECK_EQUAL(0xBAADF00D, m_pContext->R[0]);
}

TEST(watchpointTests, Set4ByteWriteWatchpoint_RunUntilThatAddressIsWritten_ButHaltsOnNextInstruction)
{
    emitNOP();
    emitNOP();
    emitSTRImmediate(R0, R1, 0);
    emitNOP();
    emitNOP();
    emitBKPT(0);
    
    char commands[64];
    snprintf(commands, sizeof(commands), "+$Z2,%x,4#", INITIAL_SP - 4);
    mockIComm_InitReceiveChecksummedData(commands, "+$c#");
    m_pContext->R[0] = 0xBAADF00D;
    m_pContext->R[1] = INITIAL_SP - 4;
        mri4simRun(mockIComm_Get(), TRUE);
    appendExpectedTPacket(SIGTRAP, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC);
    appendExpectedString("+$OK#+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());

    mockIComm_InitTransmitDataBuffer(1024);
    mockIComm_InitReceiveChecksummedData("+$c#");
    mockIComm_DelayReceiveData(6);
        mri4simRun(mockIComm_Get(), FALSE);
    resetExpectedBuffer();
    appendExpectedTPacket(SIGTRAP, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC + 6);
    appendExpectedString("+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
    CHECK_EQUAL(0xBAADF00D, IMemory_Read32(m_pContext->pMemory, INITIAL_SP - 4));
}

TEST(watchpointTests, Set4ByteReadWriteWatchpoint_RunUntilThatAddressIsWritten_ButHaltsOnNextInstruction)
{
    emitNOP();
    emitNOP();
    emitSTRImmediate(R0, R1, 0);
    emitNOP();
    emitNOP();
    emitBKPT(0);
    
    char commands[64];
    snprintf(commands, sizeof(commands), "+$Z4,%x,4#", INITIAL_SP - 4);
    mockIComm_InitReceiveChecksummedData(commands, "+$c#");
    m_pContext->R[0] = 0xBAADF00D;
    m_pContext->R[1] = INITIAL_SP - 4;
        mri4simRun(mockIComm_Get(), TRUE);
    appendExpectedTPacket(SIGTRAP, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC);
    appendExpectedString("+$OK#+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());

    mockIComm_InitTransmitDataBuffer(1024);
    mockIComm_InitReceiveChecksummedData("+$c#");
    mockIComm_DelayReceiveData(6);
        mri4simRun(mockIComm_Get(), FALSE);
    resetExpectedBuffer();
    appendExpectedTPacket(SIGTRAP, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC + 6);
    appendExpectedString("+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
    CHECK_EQUAL(0xBAADF00D, IMemory_Read32(m_pContext->pMemory, INITIAL_SP - 4));
}

TEST(watchpointTests, Set4ByteReadWriteWatchpoint_RunUntilThatAddressIsRead_ButHaltsOnNextInstruction)
{
    emitNOP();
    emitNOP();
    emitLDRImmediate(R0, R1, 0);
    emitNOP();
    emitNOP();
    emitBKPT(0);
    
    char commands[64];
    snprintf(commands, sizeof(commands), "+$Z4,%x,4#", INITIAL_SP - 4);
    mockIComm_InitReceiveChecksummedData(commands, "+$c#");
    m_pContext->R[1] = INITIAL_SP - 4;
    IMemory_Write32(m_pContext->pMemory, INITIAL_SP - 4, 0xBAADF00D);
        mri4simRun(mockIComm_Get(), TRUE);
    appendExpectedTPacket(SIGTRAP, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC);
    appendExpectedString("+$OK#+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());

    mockIComm_InitTransmitDataBuffer(1024);
    mockIComm_InitReceiveChecksummedData("+$c#");
    mockIComm_DelayReceiveData(6);
        mri4simRun(mockIComm_Get(), FALSE);
    resetExpectedBuffer();
    appendExpectedTPacket(SIGTRAP, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC + 6);
    appendExpectedString("+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
    CHECK_EQUAL(0xBAADF00D, m_pContext->R[0]);
}

TEST(watchpointTests, Set4ByteReadWatchpoint_RunUntilThatAddressIsRead_ClearAndMakeSureThatNextReadNoHit)
{
    emitNOP();
    emitNOP();
    emitLDRImmediate(R0, R1, 0);
    emitLDRImmediate(R0, R1, 0);
    emitNOP();
    emitBKPT(0);
    
    char commands[64];
    snprintf(commands, sizeof(commands), "+$Z3,%x,4#", INITIAL_SP - 4);
    mockIComm_InitReceiveChecksummedData(commands, "+$c#");
    m_pContext->R[1] = INITIAL_SP - 4;
        mri4simRun(mockIComm_Get(), TRUE);
    appendExpectedTPacket(SIGTRAP, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC);
    appendExpectedString("+$OK#+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());

    mockIComm_InitTransmitDataBuffer(1024);
    snprintf(commands, sizeof(commands), "+$z3,%x,4#", INITIAL_SP - 4);
    mockIComm_InitReceiveChecksummedData(commands, "+$c#");
    mockIComm_DelayReceiveData(6);
        mri4simRun(mockIComm_Get(), FALSE);
    resetExpectedBuffer();
    appendExpectedTPacket(SIGTRAP, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC + 6);
    appendExpectedString("+$OK#+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());

    mockIComm_InitTransmitDataBuffer(1024);
    mockIComm_InitReceiveChecksummedData("+$c#");
    mockIComm_DelayReceiveData(6);
        mri4simRun(mockIComm_Get(), FALSE);
    resetExpectedBuffer();
    appendExpectedTPacket(SIGTRAP, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC + 10);
    appendExpectedString("+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
}

TEST(watchpointTests, Set1ByteReadWatchpoint_RunUntilThatAddressIsRead_ButHaltsOnNextInstruction)
{
    emitNOP();
    emitNOP();
    emitLDRBImmediate(R0, R1, 0);
    emitNOP();
    emitNOP();
    emitBKPT(0);
    
    char commands[64];
    snprintf(commands, sizeof(commands), "+$Z3,%x,1#", INITIAL_SP - 4);
    mockIComm_InitReceiveChecksummedData(commands, "+$c#");
    m_pContext->R[1] = INITIAL_SP - 4;
    IMemory_Write8(m_pContext->pMemory, INITIAL_SP - 4, 0x5A);
        mri4simRun(mockIComm_Get(), TRUE);
    appendExpectedTPacket(SIGTRAP, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC);
    appendExpectedString("+$OK#+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());

    mockIComm_InitTransmitDataBuffer(1024);
    mockIComm_InitReceiveChecksummedData("+$c#");
    mockIComm_DelayReceiveData(6);
        mri4simRun(mockIComm_Get(), FALSE);
    resetExpectedBuffer();
    appendExpectedTPacket(SIGTRAP, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC + 6);
    appendExpectedString("+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
    CHECK_EQUAL(0x5A, m_pContext->R[0]);
}

TEST(watchpointTests, Set2ByteWriteWatchpoint_RunUntilThatAddressIsWrite_ButHaltsOnNextInstruction)
{
    emitNOP();
    emitNOP();
    emitSTRHImmediate(R0, R1, 0);
    emitNOP();
    emitNOP();
    emitBKPT(0);
    
    char commands[64];
    snprintf(commands, sizeof(commands), "+$Z2,%x,2#", INITIAL_SP - 4);
    mockIComm_InitReceiveChecksummedData(commands, "+$c#");
    m_pContext->R[0] = 0xBAADF00D;
    m_pContext->R[1] = INITIAL_SP - 4;
        mri4simRun(mockIComm_Get(), TRUE);
    appendExpectedTPacket(SIGTRAP, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC);
    appendExpectedString("+$OK#+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());

    mockIComm_InitTransmitDataBuffer(1024);
    mockIComm_InitReceiveChecksummedData("+$c#");
    mockIComm_DelayReceiveData(6);
        mri4simRun(mockIComm_Get(), FALSE);
    resetExpectedBuffer();
    appendExpectedTPacket(SIGTRAP, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC + 6);
    appendExpectedString("+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
    CHECK_EQUAL(0xF00D, IMemory_Read16(m_pContext->pMemory, INITIAL_SP - 4));
}

TEST(watchpointTests, Set8ByteWriteWatchpoint_RunUntilThatAddressIsWrite_ButHaltsOnNextInstruction)
{
    emitNOP();
    emitNOP();
    emitSTRHImmediate(R0, R1, 0);
    emitNOP();
    emitNOP();
    emitBKPT(0);
    
    char commands[64];
    snprintf(commands, sizeof(commands), "+$Z2,%x,2#", INITIAL_SP - 4);
    mockIComm_InitReceiveChecksummedData(commands, "+$c#");
    m_pContext->R[0] = 0xBAADF00D;
    m_pContext->R[1] = INITIAL_SP - 4;
        mri4simRun(mockIComm_Get(), TRUE);
    appendExpectedTPacket(SIGTRAP, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC);
    appendExpectedString("+$OK#+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());

    mockIComm_InitTransmitDataBuffer(1024);
    mockIComm_InitReceiveChecksummedData("+$c#");
    mockIComm_DelayReceiveData(6);
        mri4simRun(mockIComm_Get(), FALSE);
    resetExpectedBuffer();
    appendExpectedTPacket(SIGTRAP, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC + 6);
    appendExpectedString("+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
    CHECK_EQUAL(0xF00D, IMemory_Read16(m_pContext->pMemory, INITIAL_SP - 4));
}

TEST(watchpointTests, Set8ByteReadWatchpoint_RunUntilSecondWordIsRead_ButHaltsOnNextInstruction)
{
    emitNOP();
    emitNOP();
    emitLDRImmediate(R0, R1, 0);
    emitNOP();
    emitNOP();
    emitBKPT(0);
    
    char commands[64];
    snprintf(commands, sizeof(commands), "+$Z3,%x,8#", INITIAL_SP - 8);
    mockIComm_InitReceiveChecksummedData(commands, "+$c#");
    m_pContext->R[1] = INITIAL_SP - 4;
    IMemory_Write32(m_pContext->pMemory, INITIAL_SP - 4, 0xBAADF00D);
        mri4simRun(mockIComm_Get(), TRUE);
    appendExpectedTPacket(SIGTRAP, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC);
    appendExpectedString("+$OK#+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());

    mockIComm_InitTransmitDataBuffer(1024);
    mockIComm_InitReceiveChecksummedData("+$c#");
    mockIComm_DelayReceiveData(6);
        mri4simRun(mockIComm_Get(), FALSE);
    resetExpectedBuffer();
    appendExpectedTPacket(SIGTRAP, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC + 6);
    appendExpectedString("+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
    CHECK_EQUAL(0xBAADF00D, m_pContext->R[0]);
}

TEST(watchpointTests, FailMemoryAllocation_AttemptToSet4ByteReadWatchpoint_ShouldReturnErrorMessage)
{
    char commands[64];
    snprintf(commands, sizeof(commands), "+$Z3,%x,4#", INITIAL_SP - 4);
    mockIComm_InitReceiveChecksummedData(commands, "+$c#");
    MallocFailureInject_FailAllocation(1);
        mri4simRun(mockIComm_Get(), TRUE);
    appendExpectedTPacket(SIGTRAP, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC);
    appendExpectedString("+$" MRI_ERROR_NO_FREE_BREAKPOINT "#+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
}

TEST(watchpointTests, AttemptToSet4ByteReadWatchpoint_AtInvalidAddress_ShouldReturnErrorMessage)
{
    mockIComm_InitReceiveChecksummedData("+$Z3,fffffffc,4#", "+$c#");
        mri4simRun(mockIComm_Get(), TRUE);
    appendExpectedTPacket(SIGTRAP, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC);
    appendExpectedString("+$" MRI_ERROR_NO_FREE_BREAKPOINT "#+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
}

TEST(watchpointTests, AttemptToClear4ByteReadWatchpoint_AtInvalidAddress_ShouldReturnErrorMessage)
{
    mockIComm_InitReceiveChecksummedData("+$z3,fffffffc,4#", "+$c#");
        mri4simRun(mockIComm_Get(), TRUE);
    appendExpectedTPacket(SIGTRAP, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC);
    appendExpectedString("+$" MRI_ERROR_INVALID_ARGUMENT "#+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
}

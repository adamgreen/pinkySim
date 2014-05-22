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
    #include <mockFileIo.h>
    #include <NewLibSemihost.h>
}
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include "mri4simBaseTest.h"

TEST_GROUP_BASE(semihostTests, mri4simBase)
{
    void setup()
    {
        mri4simBase::setup();
        mockFileIo_CreateWriteBuffer(32);
        m_pContext->R[0] = 1;
        m_pContext->R[1] = 2;
        m_pContext->R[2] = 3;
        m_pContext->R[3] = 4;
    }

    void teardown()
    {
        MallocFailureInject_Restore();
        mockFileIo_Uninit();
        mri4simBase::teardown();
    }

    void copyBufferToSimulator(uint32_t address, const void* pv, size_t size)
    {
        const uint8_t* pSrc = (const uint8_t*)pv;
        while (size--)
        {
            IMemory_Write8(m_pContext->pMemory, address, *pSrc);
            address++;
            pSrc++;
        }
    }
    void validateBytesInSimulator(uint32_t address, const char* pExpected, size_t len)
    {
        while (*pExpected)
            CHECK_EQUAL(*pExpected++, IMemory_Read8(m_pContext->pMemory, address++));
    }

};


TEST(semihostTests, WriteCall_NotConsole_VerifyReturnCodeInR0)
{
    m_pContext->R[0] = 4;
    emitBKPT(NEWLIB_WRITE);
    emitBKPT(0);

    mockIComm_InitReceiveChecksummedData("+$F5#", "+$c#");
    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get(), FALSE);
    appendExpectedString("$Fwrite,04,02,03#+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
    CHECK_EQUAL(5, m_pContext->R[0]);
}

TEST(semihostTests, WriteCall_NotConsole_VerifyReturnCodeInR0AndErrnoInR1)
{
    m_pContext->R[0] = 4;
    emitBKPT(NEWLIB_WRITE);
    emitBKPT(0);

    mockIComm_InitReceiveChecksummedData("+$F-1,5#", "+$c#");
    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get(), FALSE);
    appendExpectedString("$Fwrite,04,02,03#+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
    CHECK_EQUAL((uint32_t)-1, m_pContext->R[0]);
    CHECK_EQUAL(5, m_pContext->R[1]);
}

TEST(semihostTests, WriteCall_StdOut_VerifyTextSentToConsoleAndGdb)
{
    const char buffer[] = "Test\n";
    m_pContext->R[0] = STDOUT_FILENO;
    m_pContext->R[1] = INITIAL_SP - sizeof(buffer) + 1;
    m_pContext->R[2] = sizeof(buffer) - 1;
    copyBufferToSimulator(m_pContext->R[1], buffer, m_pContext->R[2]);

    emitBKPT(NEWLIB_WRITE);
    emitBKPT(0);

    mockIComm_InitReceiveChecksummedData("+$F5#", "+$c#");
    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get(), FALSE);
    char expectedResult[64];
    snprintf(expectedResult, sizeof(expectedResult),
             "$Fwrite,01,%08lx,%02lx#+",
             INITIAL_SP - sizeof(buffer) + 1,
             sizeof(buffer) - 1);
    appendExpectedString(expectedResult);
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
    STRCMP_EQUAL("Test\n", mockFileIo_GetStdOutData());
    CHECK_EQUAL(5, m_pContext->R[0]);
}

TEST(semihostTests, WriteCall_StdErr_VerifyTextSentToConsoleAndGdb)
{
    const char buffer[] = "Test\n";
    m_pContext->R[0] = STDERR_FILENO;
    m_pContext->R[1] = INITIAL_SP - sizeof(buffer) + 1;
    m_pContext->R[2] = sizeof(buffer) - 1;
    copyBufferToSimulator(m_pContext->R[1], buffer, m_pContext->R[2]);

    emitBKPT(NEWLIB_WRITE);
    emitBKPT(0);

    mockIComm_InitReceiveChecksummedData("+$F5#", "+$c#");
    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get(), FALSE);
    char expectedResult[64];
    snprintf(expectedResult, sizeof(expectedResult),
             "$Fwrite,02,%08lx,%02lx#+",
             INITIAL_SP - sizeof(buffer) + 1,
             sizeof(buffer) - 1);
    appendExpectedString(expectedResult);
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
    STRCMP_EQUAL("Test\n", mockFileIo_GetStdErrData());
    CHECK_EQUAL(5, m_pContext->R[0]);
}

TEST(semihostTests, WriteCall_StdOut_GdbNotConnected_VerifyTextSentToConsoleOnly)
{
    const char buffer[] = "Test\n";
    m_pContext->R[0] = STDOUT_FILENO;
    m_pContext->R[1] = INITIAL_SP - sizeof(buffer) + 1;
    m_pContext->R[2] = sizeof(buffer) - 1;
    copyBufferToSimulator(m_pContext->R[1], buffer, m_pContext->R[2]);
    mockIComm_SetIsGdbConnectedFlag(0);

    emitBKPT(NEWLIB_WRITE);
    emitBKPT(0);

    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get(), FALSE);
    STRCMP_EQUAL("", mockIComm_GetTransmittedData());
    STRCMP_EQUAL("Test\n", mockFileIo_GetStdOutData());
    CHECK_EQUAL(5, m_pContext->R[0]);
}

TEST(semihostTests, ReadCall_VerifyReturnCodeInR0)
{
    emitBKPT(NEWLIB_READ);
    emitBKPT(0);

    mockIComm_InitReceiveChecksummedData("+$F5#", "+$c#");
    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get(), FALSE);
    appendExpectedString("$Fread,01,02,03#+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
    CHECK_EQUAL(5, m_pContext->R[0]);
}

TEST(semihostTests, ReadCall_StdIn_GdbNotConnected_ReadFromConsoleInsteadOfGdb)
{
    const char testString[] = "Test\n";
    m_pContext->R[0] = STDIN_FILENO;
    m_pContext->R[1] = INITIAL_SP - sizeof(testString) + 1;
    m_pContext->R[2] = sizeof(testString) - 1;
    mockFileIo_SetReadData(testString, sizeof(testString) - 1);
    mockIComm_SetIsGdbConnectedFlag(0);

    emitBKPT(NEWLIB_READ);
    emitBKPT(0);

    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get(), FALSE);
    STRCMP_EQUAL("", mockIComm_GetTransmittedData());
    CHECK_EQUAL((uint32_t)sizeof(testString) - 1, m_pContext->R[0]);
    validateBytesInSimulator(INITIAL_SP - sizeof(testString) + 1, testString, sizeof(testString) - 1);
}

TEST(semihostTests, ReadCall_StdIn_GdbNotConnected_FailMemoryAllocation_ShouldReturnError)
{
    const char testString[] = "Test\n";
    m_pContext->R[0] = STDIN_FILENO;
    m_pContext->R[1] = INITIAL_SP - sizeof(testString) + 1;
    m_pContext->R[2] = sizeof(testString) - 1;
    mockFileIo_SetReadData(testString, sizeof(testString) - 1);
    mockIComm_SetIsGdbConnectedFlag(0);
    MallocFailureInject_FailAllocation(1);


    emitBKPT(NEWLIB_READ);
    emitBKPT(0);

    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get(), FALSE);
    STRCMP_EQUAL("", mockIComm_GetTransmittedData());
    CHECK_EQUAL((uint32_t)-1, m_pContext->R[0]);
    CHECK_EQUAL(ENOMEM, m_pContext->R[1]);
}

TEST(semihostTests, ReadCall_StdIn_GdbNotConnected_FailReadCall_ShouldReturnError)
{
    const char testString[] = "Test\n";
    m_pContext->R[0] = STDIN_FILENO;
    m_pContext->R[1] = INITIAL_SP - sizeof(testString) + 1;
    m_pContext->R[2] = sizeof(testString) - 1;
    mockFileIo_SetReadData(testString, sizeof(testString) - 1);
    mockIComm_SetIsGdbConnectedFlag(0);
    mockFileIo_SetReadToFail(-1, EFAULT);


    emitBKPT(NEWLIB_READ);
    emitBKPT(0);

    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get(), FALSE);
    STRCMP_EQUAL("", mockIComm_GetTransmittedData());
    CHECK_EQUAL((uint32_t)-1, m_pContext->R[0]);
    CHECK_EQUAL(EFAULT, m_pContext->R[1]);
}

TEST(semihostTests, OpenCall_VerifyReturnCodeInR0)
{
    emitBKPT(NEWLIB_OPEN);
    emitBKPT(0);

    mockIComm_InitReceiveChecksummedData("+$F5#", "+$c#");
    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get(), FALSE);
    appendExpectedString("$Fopen,01/04,02,03#+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
    CHECK_EQUAL(5, m_pContext->R[0]);
}

TEST(semihostTests, RenameCall_VerifyReturnCodeInR0)
{
    emitBKPT(NEWLIB_RENAME);
    emitBKPT(0);

    mockIComm_InitReceiveChecksummedData("+$F5#", "+$c#");
    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get(), FALSE);
    appendExpectedString("$Frename,01/03,02/04#+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
    CHECK_EQUAL(5, m_pContext->R[0]);
}

TEST(semihostTests, UnlinkCall_VerifyReturnCodeInR0)
{
    emitBKPT(NEWLIB_UNLINK);
    emitBKPT(0);

    mockIComm_InitReceiveChecksummedData("+$F5#", "+$c#");
    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get(), FALSE);
    appendExpectedString("$Funlink,01/02#+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
    CHECK_EQUAL(5, m_pContext->R[0]);
}

TEST(semihostTests, StatCall_VerifyReturnCodeInR0)
{
    emitBKPT(NEWLIB_STAT);
    emitBKPT(0);

    mockIComm_InitReceiveChecksummedData("+$F5#", "+$c#");
    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get(), FALSE);
    appendExpectedString("$Fstat,01/03,02#+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
    CHECK_EQUAL(5, m_pContext->R[0]);
}

TEST(semihostTests, SeekCall_VerifyReturnCodeInR0)
{
    m_pContext->R[0] = 4;
    emitBKPT(NEWLIB_LSEEK);
    emitBKPT(0);

    mockIComm_InitReceiveChecksummedData("+$F5#", "+$c#");
    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get(), FALSE);
    appendExpectedString("$Flseek,04,02,03#+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
    CHECK_EQUAL(5, m_pContext->R[0]);
}

TEST(semihostTests, SeekCall_StdIn_ShouldReturnError)
{
    m_pContext->R[0] = STDIN_FILENO;
    emitBKPT(NEWLIB_LSEEK);
    emitBKPT(0);

    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get(), FALSE);
    STRCMP_EQUAL("", mockIComm_GetTransmittedData());
    CHECK_EQUAL((uint32_t)-1, m_pContext->R[0]);
    CHECK_EQUAL(EBADF, m_pContext->R[1]);
}

TEST(semihostTests, SeekCall_StdOud_ShouldReturnError)
{
    m_pContext->R[0] = STDOUT_FILENO;
    emitBKPT(NEWLIB_LSEEK);
    emitBKPT(0);

    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get(), FALSE);
    STRCMP_EQUAL("", mockIComm_GetTransmittedData());
    CHECK_EQUAL((uint32_t)-1, m_pContext->R[0]);
    CHECK_EQUAL(EBADF, m_pContext->R[1]);
}

TEST(semihostTests, SeekCall_StdErr_ShouldReturnError)
{
    m_pContext->R[0] = STDERR_FILENO;
    emitBKPT(NEWLIB_LSEEK);
    emitBKPT(0);

    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get(), FALSE);
    STRCMP_EQUAL("", mockIComm_GetTransmittedData());
    CHECK_EQUAL((uint32_t)-1, m_pContext->R[0]);
    CHECK_EQUAL(EBADF, m_pContext->R[1]);
}

TEST(semihostTests, CloseCall_VerifyReturnCodeInR0)
{
    m_pContext->R[0] = 3;
    emitBKPT(NEWLIB_CLOSE);
    emitBKPT(0);

    mockIComm_InitReceiveChecksummedData("+$F5#", "+$c#");
    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get(), FALSE);
    appendExpectedString("$Fclose,03#+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
    CHECK_EQUAL(5, m_pContext->R[0]);
}

TEST(semihostTests, CloseCall_StdIn_IgnoreAndReturnZero)
{
    m_pContext->R[0] = STDIN_FILENO;
    emitBKPT(NEWLIB_CLOSE);
    emitBKPT(0);

    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get(), FALSE);
    STRCMP_EQUAL("", mockIComm_GetTransmittedData());
    CHECK_EQUAL(0, m_pContext->R[0]);
}

TEST(semihostTests, CloseCall_StdOut_IgnoreAndReturnZero)
{
    m_pContext->R[0] = STDOUT_FILENO;
    emitBKPT(NEWLIB_CLOSE);
    emitBKPT(0);

    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get(), FALSE);
    STRCMP_EQUAL("", mockIComm_GetTransmittedData());
    CHECK_EQUAL(0, m_pContext->R[0]);
}

TEST(semihostTests, CloseCall_StdErr_IgnoreAndReturnZero)
{
    m_pContext->R[0] = STDERR_FILENO;
    emitBKPT(NEWLIB_CLOSE);
    emitBKPT(0);

    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get(), FALSE);
    STRCMP_EQUAL("", mockIComm_GetTransmittedData());
    CHECK_EQUAL(0, m_pContext->R[0]);
}

TEST(semihostTests, FStatCall_VerifyReturnCodeInR0)
{
    m_pContext->R[0] = 3;
    emitBKPT(NEWLIB_FSTAT);
    emitBKPT(0);

    mockIComm_InitReceiveChecksummedData("+$F5#", "+$c#");
    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get(), FALSE);
    appendExpectedString("$Ffstat,03,02#+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
    CHECK_EQUAL(5, m_pContext->R[0]);
}

TEST(semihostTests, FStatCall_StdIn_ReturnErrorNegativeOne)
{
    m_pContext->R[0] = STDIN_FILENO;
    emitBKPT(NEWLIB_FSTAT);
    emitBKPT(0);

    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get(), FALSE);
    STRCMP_EQUAL("", mockIComm_GetTransmittedData());
    CHECK_EQUAL((uint32_t)-1, m_pContext->R[0]);
}

TEST(semihostTests, FStatCall_StdOut_ReturnErrorNegativeOne)
{
    m_pContext->R[0] = STDOUT_FILENO;
    emitBKPT(NEWLIB_FSTAT);
    emitBKPT(0);

    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get(), FALSE);
    STRCMP_EQUAL("", mockIComm_GetTransmittedData());
    CHECK_EQUAL((uint32_t)-1, m_pContext->R[0]);
}

TEST(semihostTests, FStatCall_StdErr_ReturnErrorNegativeOne)
{
    m_pContext->R[0] = STDERR_FILENO;
    emitBKPT(NEWLIB_FSTAT);
    emitBKPT(0);

    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get(), FALSE);
    STRCMP_EQUAL("", mockIComm_GetTransmittedData());
    CHECK_EQUAL((uint32_t)-1, m_pContext->R[0]);
}

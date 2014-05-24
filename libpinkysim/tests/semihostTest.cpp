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
        void*          pDest = MemorySim_MapSimulatedAddressToHostAddressForWrite(m_pContext->pMemory, address, size);
        memcpy(pDest, pSrc, size);
    }

    void validateBytesInSimulator(uint32_t address, const void* pExpected, size_t len)
    {
        const void* pSimulator = MemorySim_MapSimulatedAddressToHostAddressForRead(m_pContext->pMemory, address, len);
        CHECK(0 == memcmp(pSimulator, pExpected, len));
    }

};


TEST(semihostTests, WriteCall_NotConsole_VerifyTextWrittenToRegularFile)
{
    const char buffer[] = "Test\n";
    m_pContext->R[0] = 4;
    m_pContext->R[1] = INITIAL_SP - sizeof(buffer) + 1;
    m_pContext->R[2] = sizeof(buffer) - 1;
    copyBufferToSimulator(m_pContext->R[1], buffer, m_pContext->R[2]);

    emitBKPT(NEWLIB_WRITE);
    emitBKPT(0);

    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get(), FALSE);
    STRCMP_EQUAL("", mockIComm_GetTransmittedData());
    STRCMP_EQUAL("Test\n", mockFileIo_GetRegularFileData());
    CHECK_EQUAL(5, m_pContext->R[0]);
}

TEST(semihostTests, WriteCall_NotConsole_VerifyReturnCodeInR0AndErrnoInR1)
{
    m_pContext->R[0] = 4;
    m_pContext->R[1] = INITIAL_SP - 4;
    m_pContext->R[2] = 4;
    mockFileIo_SetWriteToFail(-1, EFAULT);

    emitBKPT(NEWLIB_WRITE);
    emitBKPT(0);

    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get(), FALSE);
    STRCMP_EQUAL("", mockIComm_GetTransmittedData());
    STRCMP_EQUAL("", mockFileIo_GetRegularFileData());
    CHECK_EQUAL((uint32_t)-1, m_pContext->R[0]);
    CHECK_EQUAL(EFAULT, m_pContext->R[1]);
}

TEST(semihostTests, WriteCall_NotConsole_UseInvalidBufferAddress_ShouldReturnError)
{
    m_pContext->R[0] = 4;
    m_pContext->R[1] = 0xFFFFFFF0;
    m_pContext->R[2] = 4;

    emitBKPT(NEWLIB_WRITE);
    emitBKPT(0);

    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get(), FALSE);
    STRCMP_EQUAL("", mockIComm_GetTransmittedData());
    STRCMP_EQUAL("", mockFileIo_GetRegularFileData());
    CHECK_EQUAL((uint32_t)-1, m_pContext->R[0]);
    CHECK_EQUAL(EFAULT, m_pContext->R[1]);
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

TEST(semihostTests, ReadCall_RegularFile_VerifyReturnCodeInR0)
{
    const char testString[] = "Test\n";
    m_pContext->R[0] = 4;
    m_pContext->R[1] = INITIAL_SP - sizeof(testString) + 1;
    m_pContext->R[2] = sizeof(testString) - 1;
    mockFileIo_SetReadData(testString, sizeof(testString) - 1);

    emitBKPT(NEWLIB_READ);
    emitBKPT(0);

    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get(), FALSE);
    STRCMP_EQUAL("", mockIComm_GetTransmittedData());
    CHECK_EQUAL((uint32_t)sizeof(testString) - 1, m_pContext->R[0]);
    validateBytesInSimulator(INITIAL_SP - sizeof(testString) + 1, testString, sizeof(testString) - 1);
}

TEST(semihostTests, ReadCall_RegularFile_HandleReturnedError)
{
    const char testString[] = "Test\n";
    m_pContext->R[0] = 4;
    m_pContext->R[1] = INITIAL_SP - sizeof(testString) + 1;
    m_pContext->R[2] = sizeof(testString) - 1;
    mockFileIo_SetReadToFail(-1, EIO);

    emitBKPT(NEWLIB_READ);
    emitBKPT(0);

    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get(), FALSE);
    STRCMP_EQUAL("", mockIComm_GetTransmittedData());
    CHECK_EQUAL((uint32_t)-1, m_pContext->R[0]);
    CHECK_EQUAL(EIO, m_pContext->R[1]);
}

TEST(semihostTests, ReadCall_RegularFile_AttemptToUseInvalidBufferAddress_ShouldFail)
{
    const char testString[] = "Test\n";
    m_pContext->R[0] = 4;
    m_pContext->R[1] = 0xFFFFFFF0;
    m_pContext->R[2] = 4;
    mockFileIo_SetReadData(testString, sizeof(testString) - 1);

    emitBKPT(NEWLIB_READ);
    emitBKPT(0);

    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get(), FALSE);
    STRCMP_EQUAL("", mockIComm_GetTransmittedData());
    CHECK_EQUAL((uint32_t)-1, m_pContext->R[0]);
    CHECK_EQUAL(EFAULT, m_pContext->R[1]);
}

TEST(semihostTests, ReadCall_StdIn_GdbConnected_ReadFromGdb)
{
    m_pContext->R[0] = STDIN_FILENO;
    emitBKPT(NEWLIB_READ);
    emitBKPT(0);

    mockIComm_InitReceiveChecksummedData("+$F5#", "+$c#");
    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get(), FALSE);
    appendExpectedString("$Fread,00,02,03#+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
    CHECK_EQUAL(5, m_pContext->R[0]);
}

TEST(semihostTests, ReadCall_StdIn_GdbConnected_HandleErrorFromGdb)
{
    m_pContext->R[0] = STDIN_FILENO;
    emitBKPT(NEWLIB_READ);
    emitBKPT(0);

    mockIComm_InitReceiveChecksummedData("+$F-1,5#", "+$c#");
    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get(), FALSE);
    appendExpectedString("$Fread,00,02,03#+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
    CHECK_EQUAL((uint32_t)-1, m_pContext->R[0]);
    CHECK_EQUAL(5, m_pContext->R[1]);
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

TEST(semihostTests, OpenCall_VerifyErrorCodeReturnedInR0andR1)
{
    const char testFilename[] = "foo.bar";
    m_pContext->R[0] = INITIAL_SP - sizeof(testFilename);
    m_pContext->R[1] = 2;
    m_pContext->R[2] = 3;
    m_pContext->R[3] = sizeof(testFilename);
    copyBufferToSimulator(m_pContext->R[0], testFilename, m_pContext->R[3]);
    mockFileIo_SetOpenToFail(-1, EIO);

    emitBKPT(NEWLIB_OPEN);
    emitBKPT(0);

    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get(), FALSE);
    STRCMP_EQUAL("", mockIComm_GetTransmittedData());
    CHECK_EQUAL((uint32_t)-1, m_pContext->R[0]);
    CHECK_EQUAL(EIO, m_pContext->R[1]);
}

TEST(semihostTests, OpenCall_AttemptToUseInvalidFilenamePointer_ShouldFail)
{
    m_pContext->R[0] = 0xFFFFFFF0;
    m_pContext->R[1] = 2;
    m_pContext->R[2] = 3;
    m_pContext->R[3] = 4;
    mockFileIo_SetOpenToFail(0, 0);

    emitBKPT(NEWLIB_OPEN);
    emitBKPT(0);

    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get(), FALSE);
    STRCMP_EQUAL("", mockIComm_GetTransmittedData());
    CHECK_EQUAL((uint32_t)-1, m_pContext->R[0]);
    CHECK_EQUAL(EFAULT, m_pContext->R[1]);
}

TEST(semihostTests, RenameCall_VerifyErrorsReturnInR0andR1)
{
    m_pContext->R[0] = INITIAL_SP - 4;
    m_pContext->R[1] = INITIAL_SP - 4;
    m_pContext->R[2] = 4;
    m_pContext->R[3] = 4;
    mockFileIo_SetRenameToFail(-1, EIO);

    emitBKPT(NEWLIB_RENAME);
    emitBKPT(0);

    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get(), FALSE);
    STRCMP_EQUAL("", mockIComm_GetTransmittedData());
    CHECK_EQUAL((uint32_t)-1, m_pContext->R[0]);
    CHECK_EQUAL(EIO, m_pContext->R[1]);
}

TEST(semihostTests, RenameCall_AttemptToUseInvalidFilenamePointer_ShouldFail)
{
    m_pContext->R[0] = INITIAL_SP - 4;
    m_pContext->R[1] = 0xFFFFFFF0;
    m_pContext->R[2] = 4;
    m_pContext->R[3] = 4;
    mockFileIo_SetRenameToFail(0, 0);

    emitBKPT(NEWLIB_RENAME);
    emitBKPT(0);

    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get(), FALSE);
    STRCMP_EQUAL("", mockIComm_GetTransmittedData());
    CHECK_EQUAL((uint32_t)-1, m_pContext->R[0]);
    CHECK_EQUAL(EFAULT, m_pContext->R[1]);
}

TEST(semihostTests, UnlinkCall_VerifyErrorsReturnInR0andR1)
{
    m_pContext->R[0] = INITIAL_SP - 4;
    m_pContext->R[1] = 4;
    mockFileIo_SetUnlinkToFail(-1, EIO);

    emitBKPT(NEWLIB_UNLINK);
    emitBKPT(0);

    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get(), FALSE);
    STRCMP_EQUAL("", mockIComm_GetTransmittedData());
    CHECK_EQUAL((uint32_t)-1, m_pContext->R[0]);
    CHECK_EQUAL(EIO, m_pContext->R[1]);
}

TEST(semihostTests, UnlinkCall_AttemptToUseInvalidFilenamePointer_ShouldFail)
{
    m_pContext->R[0] = 0xFFFFFFF0;
    m_pContext->R[1] = 4;
    mockFileIo_SetUnlinkToFail(0, 0);

    emitBKPT(NEWLIB_UNLINK);
    emitBKPT(0);

    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get(), FALSE);
    STRCMP_EQUAL("", mockIComm_GetTransmittedData());
    CHECK_EQUAL((uint32_t)-1, m_pContext->R[0]);
    CHECK_EQUAL(EFAULT, m_pContext->R[1]);
}

TEST(semihostTests, StatCall_VerifyReturnValueAndOutputStructure)
{
    NewlibStat  newlibStat;
    struct stat hostStat;

    const char testFilename[] = "foo.bar";
    m_pContext->R[0] = INITIAL_SP - sizeof(testFilename);
    m_pContext->R[1] = INITIAL_SP - sizeof(testFilename) - sizeof(newlibStat);
    m_pContext->R[2] = sizeof(testFilename);
    copyBufferToSimulator(m_pContext->R[0], testFilename, m_pContext->R[2]);
    hostStat.st_mode = 0x1234;
    hostStat.st_size = 0xbaadf00d;
    hostStat.st_blksize = 0x87654321;
    hostStat.st_blocks = 0xfeedfeed;
    mockFileIo_SetStatCallResults(0, 0, &hostStat);

    emitBKPT(NEWLIB_STAT);
    emitBKPT(0);

    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get(), FALSE);
    STRCMP_EQUAL("", mockIComm_GetTransmittedData());
    CHECK_EQUAL(0, m_pContext->R[0]);
    NewlibStat expected = {0x1234, 0xbaadf00d, 0x87654321, 0xfeedfeed};
    validateBytesInSimulator(INITIAL_SP - sizeof(testFilename) - sizeof(newlibStat), &expected, sizeof(expected));
}

TEST(semihostTests, StatCall_VerifyErrorReturnsInR0andR1)
{
    NewlibStat newlibStat;
    const char testFilename[] = "foo.bar";
    m_pContext->R[0] = INITIAL_SP - sizeof(testFilename);
    m_pContext->R[1] = INITIAL_SP - sizeof(testFilename) - sizeof(newlibStat);
    m_pContext->R[2] = sizeof(testFilename);
    copyBufferToSimulator(m_pContext->R[0], testFilename, m_pContext->R[2]);
    mockFileIo_SetStatCallResults(-1, EIO, NULL);

    emitBKPT(NEWLIB_STAT);
    emitBKPT(0);

    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get(), FALSE);
    STRCMP_EQUAL("", mockIComm_GetTransmittedData());
    CHECK_EQUAL((uint32_t)-1, m_pContext->R[0]);
    CHECK_EQUAL(EIO, m_pContext->R[1]);
}

TEST(semihostTests, StatCall_WithInvalidFilenamePointer_ShouldFail)
{
    NewlibStat newlibStat;
    m_pContext->R[0] = 0xFFFFFFF0;
    m_pContext->R[1] = INITIAL_SP - sizeof(newlibStat);
    m_pContext->R[2] = 4;
    mockFileIo_SetStatCallResults(0, 0, NULL);

    emitBKPT(NEWLIB_STAT);
    emitBKPT(0);

    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get(), FALSE);
    STRCMP_EQUAL("", mockIComm_GetTransmittedData());
    CHECK_EQUAL((uint32_t)-1, m_pContext->R[0]);
    CHECK_EQUAL(EFAULT, m_pContext->R[1]);
}

TEST(semihostTests, StatCall_WithInvalidBufferPointer_ShouldFail)
{
    const char testFilename[] = "foo.bar";
    m_pContext->R[0] = INITIAL_SP - sizeof(testFilename);
    m_pContext->R[1] = 0xFFFFFF00;
    m_pContext->R[2] = sizeof(testFilename);
    copyBufferToSimulator(m_pContext->R[0], testFilename, m_pContext->R[2]);
    mockFileIo_SetStatCallResults(0, 0, NULL);

    emitBKPT(NEWLIB_STAT);
    emitBKPT(0);

    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get(), FALSE);
    STRCMP_EQUAL("", mockIComm_GetTransmittedData());
    CHECK_EQUAL((uint32_t)-1, m_pContext->R[0]);
    CHECK_EQUAL(EFAULT, m_pContext->R[1]);
}

TEST(semihostTests, FStatCall_VerifyReturnValueAndOutputStructure)
{
    NewlibStat  newlibStat;
    struct stat hostStat;

    m_pContext->R[0] = 4;
    m_pContext->R[1] = INITIAL_SP - sizeof(newlibStat);
    hostStat.st_mode = 0x1234;
    hostStat.st_size = 0xbaadf00d;
    hostStat.st_blksize = 0x87654321;
    hostStat.st_blocks = 0xfeedfeed;
    mockFileIo_SetFStatCallResults(0, 0, &hostStat);

    emitBKPT(NEWLIB_FSTAT);
    emitBKPT(0);

    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get(), FALSE);
    STRCMP_EQUAL("", mockIComm_GetTransmittedData());
    CHECK_EQUAL(0, m_pContext->R[0]);
    NewlibStat expected = {0x1234, 0xbaadf00d, 0x87654321, 0xfeedfeed};
    validateBytesInSimulator(INITIAL_SP - sizeof(newlibStat), &expected, sizeof(expected));
}

TEST(semihostTests, FStatCall_VerifyErrorReturnsInR0andR1)
{
    NewlibStat newlibStat;
    m_pContext->R[0] = 4;
    m_pContext->R[1] = INITIAL_SP - sizeof(newlibStat);
    mockFileIo_SetFStatCallResults(-1, EIO, NULL);

    emitBKPT(NEWLIB_FSTAT);
    emitBKPT(0);

    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get(), FALSE);
    STRCMP_EQUAL("", mockIComm_GetTransmittedData());
    CHECK_EQUAL((uint32_t)-1, m_pContext->R[0]);
    CHECK_EQUAL(EIO, m_pContext->R[1]);
}

TEST(semihostTests, FStatCall_WithInvalidBufferPointer_ShouldFail)
{
    m_pContext->R[0] = 4;
    m_pContext->R[1] = 0xFFFFFF00;
    mockFileIo_SetFStatCallResults(0, 0, NULL);

    emitBKPT(NEWLIB_FSTAT);
    emitBKPT(0);

    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get(), FALSE);
    STRCMP_EQUAL("", mockIComm_GetTransmittedData());
    CHECK_EQUAL((uint32_t)-1, m_pContext->R[0]);
    CHECK_EQUAL(EFAULT, m_pContext->R[1]);
}

TEST(semihostTests, SeekCall_VerifyReturnErrorsInR0andR1)
{
    m_pContext->R[0] = 4;
    mockFileIo_SetLSeekToFail(-1, EIO);

    emitBKPT(NEWLIB_LSEEK);
    emitBKPT(0);

    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get(), FALSE);
    STRCMP_EQUAL("", mockIComm_GetTransmittedData());
    CHECK_EQUAL((uint32_t)-1, m_pContext->R[0]);
    CHECK_EQUAL(EIO, m_pContext->R[1]);
}

TEST(semihostTests, CloseCall_VerifyReturnErrorsInR0andR1)
{
    m_pContext->R[0] = 3;
    mockFileIo_SetCloseToFail(-1, EIO);

    emitBKPT(NEWLIB_CLOSE);
    emitBKPT(0);

    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get(), FALSE);
    STRCMP_EQUAL("", mockIComm_GetTransmittedData());
    CHECK_EQUAL((uint32_t)-1, m_pContext->R[0]);
    CHECK_EQUAL(EIO, m_pContext->R[1]);
}

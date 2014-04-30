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
#include <mri.h>
#include <NewLibSemihost.h>
#include "mri4simBaseTest.h"

TEST_GROUP_BASE(semihostTests, mri4simBase)
{
    void setup()
    {
        mri4simBase::setup();
        m_pContext->R[0] = 1;
        m_pContext->R[1] = 2;
        m_pContext->R[2] = 3;
        m_pContext->R[3] = 4;
    }

    void teardown()
    {
        mri4simBase::teardown();
    }
};


TEST(semihostTests, WriteCall_VerifyReturnCodeInR0)
{
    emitBKPT(NEWLIB_WRITE);
    emitBKPT(0);
    
    mockIComm_InitReceiveChecksummedData("+$F5#", "+$c#");
    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get(), FALSE);
    appendExpectedString("$Fwrite,01,02,03#+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
    CHECK_EQUAL(5, m_pContext->R[0]);
}

TEST(semihostTests, WriteCall_VerifyReturnCodeInR0AndErrnoInR1)
{
    emitBKPT(NEWLIB_WRITE);
    emitBKPT(0);
    
    mockIComm_InitReceiveChecksummedData("+$F-1,5#", "+$c#");
    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get(), FALSE);
    appendExpectedString("$Fwrite,01,02,03#+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
    CHECK_EQUAL((uint32_t)-1, m_pContext->R[0]);
    CHECK_EQUAL(5, m_pContext->R[1]);
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
    emitBKPT(NEWLIB_LSEEK);
    emitBKPT(0);
    
    mockIComm_InitReceiveChecksummedData("+$F5#", "+$c#");
    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get(), FALSE);
    appendExpectedString("$Flseek,01,02,03#+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
    CHECK_EQUAL(5, m_pContext->R[0]);
}

TEST(semihostTests, CloseCall_VerifyReturnCodeInR0)
{
    emitBKPT(NEWLIB_CLOSE);
    emitBKPT(0);
    
    mockIComm_InitReceiveChecksummedData("+$F5#", "+$c#");
    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get(), FALSE);
    appendExpectedString("$Fclose,01#+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
    CHECK_EQUAL(5, m_pContext->R[0]);
}

TEST(semihostTests, FStatCall_VerifyReturnCodeInR0)
{
    emitBKPT(NEWLIB_FSTAT);
    emitBKPT(0);
    
    mockIComm_InitReceiveChecksummedData("+$F5#", "+$c#");
    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get(), FALSE);
    appendExpectedString("$Ffstat,01,02#+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
    CHECK_EQUAL(5, m_pContext->R[0]);
}

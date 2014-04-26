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
    char expectedData[128];
    
    mockIComm_InitReceiveChecksummedData("+$c#");
        mri4simRun(mockIComm_Get());
    snprintf(expectedData, sizeof(expectedData),
             "$T%02x0c:%08x;0d:%08x;0e:%08x;0f:%08x;#+",
             SIGINT,
             byteSwap(0x00000000),
             byteSwap(INITIAL_SP),
             byteSwap(INITIAL_LR),
             byteSwap(INITIAL_PC));
    STRCMP_EQUAL(mockIComm_ChecksumData(expectedData), mockIComm_GetTransmittedData());
    CHECK_EQUAL(INITIAL_PC, m_pContext->pc);
}

TEST(mri4simRun, DontInterruptSimAndLetRunToBreakpoint_SendContinue_ShouldAdvancePastBreakpoint)
{
    char expectedData[128];
    
    emitNOP();
    emitBKPT(0);
    mockIComm_InitReceiveChecksummedData("+$c#");
    mockIComm_DelayReceiveData(2);
        mri4simRun(mockIComm_Get());
    snprintf(expectedData, sizeof(expectedData),
             "$T%02x0c:%08x;0d:%08x;0e:%08x;0f:%08x;#+",
             SIGTRAP,
             byteSwap(0x00000000),
             byteSwap(INITIAL_SP),
             byteSwap(INITIAL_LR),
             byteSwap(INITIAL_PC + 2));
    STRCMP_EQUAL(mockIComm_ChecksumData(expectedData), mockIComm_GetTransmittedData());
    CHECK_EQUAL(INITIAL_PC + 4, m_pContext->pc);
}

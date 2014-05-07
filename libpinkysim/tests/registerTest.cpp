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

TEST_GROUP_BASE(registerTests, mri4simBase)
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


TEST(registerTests, ReadRegisters)
{
    for (int i = 0 ; i < 13 ; i++)
        m_pContext->R[i] = 0x11111111 * i;
    m_pContext->spMain = 0xDDDDDDDD;
    m_pContext->lr = 0xEEEEEEEE;
    m_pContext->pc = 0xFFFFFFFE;
    
    mockIComm_InitReceiveChecksummedData("+$g#", "+$c#");
        mri4simRun(mockIComm_Get(), TRUE);
    appendExpectedTPacket(SIGTRAP, 0xCCCCCCCC, 0xDDDDDDDD, 0xEEEEEEEE, 0xFFFFFFFE);
    appendExpectedString("+$00000000111111112222222233333333"
                           "44444444555555556666666677777777"
                           "8888888899999999aaaaaaaabbbbbbbb"
                           "ccccccccddddddddeeeeeeeefeffffff"
                           "00000001#+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
}

TEST(registerTests, WriteRegisters)
{
    
    mockIComm_InitReceiveChecksummedData("+$G00000000111111112222222233333333"
                                            "44444444555555556666666677777777"
                                            "8888888899999999aaaaaaaabbbbbbbb"
                                            "ccccccccddddddddeeeeeeeefeffffff"
                                            "ffffffff#", "+$c#");
        mri4simRun(mockIComm_Get(), TRUE);
    appendExpectedTPacket(SIGTRAP, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC);
    appendExpectedString("+$OK#+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());

    for (int i = 0 ; i < 13 ; i++)
        CHECK_EQUAL(0x11111111U * i, m_pContext->R[i]);
    CHECK_EQUAL(0xDDDDDDDDU, m_pContext->spMain);
    CHECK_EQUAL(0xEEEEEEEEU, m_pContext->lr);
    CHECK_EQUAL(0xFFFFFFFEU, m_pContext->pc);
    CHECK_EQUAL(0xFFFFFFFFU, m_pContext->xPSR);
}

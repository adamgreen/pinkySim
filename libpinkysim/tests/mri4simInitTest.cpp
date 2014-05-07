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
// Include headers from C modules under test.
extern "C"
{
    #include <core.h>
    #include <mri4sim.h>
    #include <MemorySim.h>
}

// Include C++ headers for test harness.
#include "CppUTest/TestHarness.h"


TEST_GROUP(mri4simInit)
{
    IMemory* m_pMem;
    
    void setup()
    {
        m_pMem = MemorySim_Init();
    }

    void teardown()
    {
        CHECK_EQUAL(noException, getExceptionCode());
        MemorySim_Uninit(m_pMem);
        clearExceptionCode();
    }
};


TEST(mri4simInit, ResetVectorNotAvailable_ShouldThrowBusError)
{
    MemorySim_CreateRegion(m_pMem, FLASH_BASE_ADDRESS, sizeof(uint32_t));
    MemorySim_MakeRegionReadOnly(m_pMem, FLASH_BASE_ADDRESS);
        __try_and_catch ( mri4simInit(m_pMem) );
    CHECK_EQUAL(busErrorException, getExceptionCode());
    clearExceptionCode();
}

TEST(mri4simInit, InitialStackValueNotAvailable_ShouldThrowBusError)
{
    MemorySim_CreateRegion(m_pMem, FLASH_BASE_ADDRESS, sizeof(uint32_t) - 1);
    MemorySim_MakeRegionReadOnly(m_pMem, FLASH_BASE_ADDRESS);
        __try_and_catch ( mri4simInit(m_pMem) );
    CHECK_EQUAL(busErrorException, getExceptionCode());
    clearExceptionCode();
}

TEST(mri4simInit, ShouldInitializeContextSPandPCfromFirstTwoFlashWordsAndRestOfRegisterShouldBeZero)
{
    uint32_t flashVectors[] = {0x10008000, 0x00000101};
    MemorySim_CreateRegionsFromFlashImage(m_pMem, flashVectors, sizeof(flashVectors));
    PinkySimContext* pContext = mri4simGetContext();
    memset(pContext, 0xFF, sizeof(*pContext));
        mri4simInit(m_pMem);
    CHECK_EQUAL(0x10008000, pContext->spMain);
    CHECK_EQUAL(0x00000100, pContext->pc);
    for (int i = 0 ; i <= 12 ; i++)
        CHECK_EQUAL(0, pContext->R[i]);
    CHECK_EQUAL(0x00000000, pContext->lr);
    CHECK_EQUAL(EPSR_T, pContext->xPSR);
    CHECK_EQUAL(m_pMem, pContext->pMemory);
}

TEST(mri4simInit, MriCoreShouldBeSuccessfullyInitialized)
{
    uint32_t flashVectors[] = {0x10008000, 0x00000101};
    MemorySim_CreateRegionsFromFlashImage(m_pMem, flashVectors, sizeof(flashVectors));
        mri4simInit(m_pMem);
    CHECK_TRUE( IsFirstException() );
    CHECK_TRUE( WasSuccessfullyInit() );
}

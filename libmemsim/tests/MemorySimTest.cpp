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
    #include <MemorySim.h>
    #include <MallocFailureInject.h>
}

// Include C++ headers for test harness.
#include "CppUTest/TestHarness.h"


TEST_GROUP(MemorySim)
{
    IMemory* m_pMemory;
    
    void setup()
    {
        m_pMemory = MemorySim_Init();
    }

    void teardown()
    {
        CHECK_EQUAL(noException, getExceptionCode());
        clearExceptionCode();
        MemorySim_Uninit(m_pMemory);
        MallocFailureInject_Restore();
    }
    
    void validateExceptionThrown(int expectedExceptionCode)
    {
        CHECK_EQUAL(expectedExceptionCode, getExceptionCode());
        clearExceptionCode();
    }
};

TEST(MemorySim, BasicInitTakenCareOfInSetup)
{
    CHECK(m_pMemory != NULL);
}

TEST(MemorySim, NoMemoryRegionsSetupShouldResultInAllReadsAndWritesThrowing)
{
    __try_and_catch( IMemory_Read32(m_pMemory, 0x00000000) );
    validateExceptionThrown(busErrorException);
    __try_and_catch( IMemory_Read16(m_pMemory, 0x00000000) );
    validateExceptionThrown(busErrorException);
    __try_and_catch( IMemory_Read8(m_pMemory, 0x00000000) );
    validateExceptionThrown(busErrorException);
    __try_and_catch( IMemory_Write32(m_pMemory, 0x00000000, 0xFFFFFFFF) );
    validateExceptionThrown(busErrorException);
    __try_and_catch( IMemory_Write16(m_pMemory, 0x00000000, 0xFFFF) );
    validateExceptionThrown(busErrorException);
    __try_and_catch( IMemory_Write8(m_pMemory, 0x00000000, 0xFF) );
    validateExceptionThrown(busErrorException);
}

TEST(MemorySim, ShouldThrowIfOutOfMemory)
{
    // Each region is two allocations.
    // 1. The MemoryRegion structure which describes the region.
    // 2. The array of bytes used to simulate the memory.
    static const size_t allocationsToFail = 2;
    size_t i;
    
    for (i = 1 ; i <= allocationsToFail ; i++)
    {
        MallocFailureInject_FailAllocation(i);
        __try_and_catch( MemorySim_CreateRegion(m_pMemory, 0x00000004, 4) );
        validateExceptionThrown(outOfMemoryException);
    }
    MallocFailureInject_FailAllocation(i);
    MemorySim_CreateRegion(m_pMemory, 0x00000004, 4);
}

TEST(MemorySim, SimulateFourBytes_ShouldBeZeroFilledByDefault)
{
    MemorySim_CreateRegion(m_pMemory, 0x00000004, 4);
    CHECK_EQUAL(0x0000000, IMemory_Read32(m_pMemory, 0x00000004));
}

TEST(MemorySim, SimulateFourBytes_DefaultsToReadWrite_VerifyCanReadAndWrite)
{
    static const uint32_t testAddress = 0x00000004;
    MemorySim_CreateRegion(m_pMemory, testAddress, 4);

    IMemory_Write32(m_pMemory, testAddress, 0x11111111);
    CHECK_EQUAL(0x11111111, IMemory_Read32(m_pMemory, testAddress));
    IMemory_Write16(m_pMemory, testAddress, 0x2222);
    CHECK_EQUAL(0x2222, IMemory_Read16(m_pMemory, testAddress));
    IMemory_Write8(m_pMemory, testAddress, 0x33);
    CHECK_EQUAL(0x33, IMemory_Read8(m_pMemory, testAddress));
}

TEST(MemorySim, SimulateFourBytes_VerifyReadWritesOfPreviousWordThrows)
{
    static const uint32_t testAddress = 0x00000004;
    MemorySim_CreateRegion(m_pMemory, testAddress, 4);

    __try_and_catch( IMemory_Write32(m_pMemory, testAddress - 4, 0x11111111) );
    validateExceptionThrown(busErrorException);
    __try_and_catch( IMemory_Read32(m_pMemory, testAddress - 4) );
    validateExceptionThrown(busErrorException);
    __try_and_catch( IMemory_Write16(m_pMemory, testAddress - 2, 0x2222) );
    validateExceptionThrown(busErrorException);
    __try_and_catch( IMemory_Read16(m_pMemory, testAddress - 2) );
    validateExceptionThrown(busErrorException);
    __try_and_catch( IMemory_Write8(m_pMemory, testAddress - 1, 0x33) );
    validateExceptionThrown(busErrorException);
    __try_and_catch( IMemory_Read8(m_pMemory, testAddress - 1) );
    validateExceptionThrown(busErrorException);
}

TEST(MemorySim, SimulateFourBytes_VerifyReadWritesOfNextWordThrows)
{
    static const uint32_t testAddress = 0x00000004;
    MemorySim_CreateRegion(m_pMemory, testAddress, 4);

    __try_and_catch( IMemory_Write32(m_pMemory, testAddress + 4, 0x11111111) );
    validateExceptionThrown(busErrorException);
    __try_and_catch( IMemory_Read32(m_pMemory, testAddress + 4) );
    validateExceptionThrown(busErrorException);
    __try_and_catch( IMemory_Write16(m_pMemory, testAddress + 4, 0x2222) );
    validateExceptionThrown(busErrorException);
    __try_and_catch( IMemory_Read16(m_pMemory, testAddress + 4) );
    validateExceptionThrown(busErrorException);
    __try_and_catch( IMemory_Write8(m_pMemory, testAddress + 4, 0x33) );
    validateExceptionThrown(busErrorException);
    __try_and_catch( IMemory_Read8(m_pMemory, testAddress + 4) );
    validateExceptionThrown(busErrorException);
}


TEST(MemorySim, SimulateFourBytes_VerifyOverlappingReadWritesWillThrow)
{
    static const uint32_t testAddress = 0x00000004;
    MemorySim_CreateRegion(m_pMemory, testAddress, 4);

    __try_and_catch( IMemory_Write32(m_pMemory, testAddress + 1, 0x11111111) );
    validateExceptionThrown(busErrorException);
    __try_and_catch( IMemory_Read32(m_pMemory, testAddress + 1) );
    validateExceptionThrown(busErrorException);
    __try_and_catch( IMemory_Write16(m_pMemory, testAddress + 3, 0x2222) );
    validateExceptionThrown(busErrorException);
    __try_and_catch( IMemory_Read16(m_pMemory, testAddress + 3) );
    validateExceptionThrown(busErrorException);
}

TEST(MemorySim, SimulateFourBytes_VerifyCanMakeReadOnly)
{
    static const uint32_t testAddress = 0x00000004;
    MemorySim_CreateRegion(m_pMemory, testAddress, 4);
    MemorySim_MakeRegionReadOnly(m_pMemory, testAddress);

    __try_and_catch( IMemory_Write32(m_pMemory, testAddress, 0x11111111) );
    validateExceptionThrown(busErrorException);
    __try_and_catch( IMemory_Write16(m_pMemory, testAddress, 0x2222) );
    validateExceptionThrown(busErrorException);
    __try_and_catch( IMemory_Write8(m_pMemory, testAddress, 0x33) );
    validateExceptionThrown(busErrorException);

    CHECK_EQUAL(0x00000000, IMemory_Read32(m_pMemory, testAddress));
    CHECK_EQUAL(0x0000, IMemory_Read16(m_pMemory, testAddress));
    CHECK_EQUAL(0x00, IMemory_Read8(m_pMemory, testAddress));
}

TEST(MemorySim, SimulateFourBytes_VerifyCanReadBothHalfWords)
{
    static const uint32_t testAddress = 0x00000004;
    MemorySim_CreateRegion(m_pMemory, testAddress, 4);

    IMemory_Write32(m_pMemory, testAddress, 0x12345678);
    CHECK_EQUAL(0x5678, IMemory_Read16(m_pMemory, testAddress));
    CHECK_EQUAL(0x1234, IMemory_Read16(m_pMemory, testAddress + 2));
}

TEST(MemorySim, SimulateFourBytes_VerifyCanReadAllFourBytes)
{
    static const uint32_t testAddress = 0x00000004;
    MemorySim_CreateRegion(m_pMemory, testAddress, 4);

    IMemory_Write32(m_pMemory, testAddress, 0x12345678);
    CHECK_EQUAL(0x78, IMemory_Read8(m_pMemory, testAddress));
    CHECK_EQUAL(0x56, IMemory_Read8(m_pMemory, testAddress + 1));
    CHECK_EQUAL(0x34, IMemory_Read8(m_pMemory, testAddress + 2));
    CHECK_EQUAL(0x12, IMemory_Read8(m_pMemory, testAddress + 3));
}

TEST(MemorySim, SimulateLargerRegion_WriteReadAllWords)
{
    static const uint32_t baseAddress = 0x10000000;
    static const uint32_t size = 64 * 1024;
    static const uint32_t wordCount = size / sizeof(uint32_t);
    uint32_t i;
    uint32_t testValue;
    uint32_t address;
    
    MemorySim_CreateRegion(m_pMemory, baseAddress, size);

    for (i = 0, address = baseAddress, testValue = 0xFFFFFFFF ; i < wordCount ; i++, testValue--, address += 4)
        IMemory_Write32(m_pMemory, address, testValue);

    for (i = 0, address = baseAddress, testValue = 0xFFFFFFFF ; i < wordCount ; i++, testValue--, address += 4)
        CHECK_EQUAL(testValue, IMemory_Read32(m_pMemory, address));
}

TEST(MemorySim, SimulateTwoMemoryRegions)
{
    static const uint32_t region1 = 0x00000000;
    static const uint32_t region2 = 0x10000000;
    MemorySim_CreateRegion(m_pMemory, region1, 4);
    MemorySim_CreateRegion(m_pMemory, region2, 4);

    IMemory_Write32(m_pMemory, region1, 0x11111111);
    CHECK_EQUAL(0x11111111, IMemory_Read32(m_pMemory, region1));
    IMemory_Write32(m_pMemory, region2, 0x22222222);
    CHECK_EQUAL(0x22222222, IMemory_Read32(m_pMemory, region2));
}

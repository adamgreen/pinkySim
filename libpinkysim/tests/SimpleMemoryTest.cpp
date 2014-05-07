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
    #include <SimpleMemory.h>
}

// Include C++ headers for test harness.
#include "CppUTest/TestHarness.h"
#include "pinkySimBaseTest.h"


TEST_GROUP(SimpleMemory)
{
    IMemory* m_pMemory;

    void setup()
    {
        m_pMemory = SimpleMemory_Init();
    }

    void teardown()
    {
        clearExceptionCode();
    }
};

TEST(SimpleMemory, BasicInit)
{
    CHECK(m_pMemory != NULL);
}

TEST(SimpleMemory, TooManySetMemoryCalls)
{
    for (int i = 0 ; i < SIMPLE_MEMORY_REGION_COUNT ; i++)
        SimpleMemory_SetMemory(m_pMemory, i*4, i, READ_WRITE);
    CHECK_EQUAL(noException, getExceptionCode());

    __try_and_catch(SimpleMemory_SetMemory(m_pMemory, 0x10000000, 0, READ_WRITE));
    CHECK_EQUAL(outOfMemoryException, getExceptionCode());
}

TEST(SimpleMemory, AttemptToRead32UnsetMemory)
{
    __try_and_catch(IMemory_Read32(m_pMemory, 0x10000000));
    CHECK_EQUAL(busErrorException, getExceptionCode());
}

TEST(SimpleMemory, ValidRead32Calls)
{
    SimpleMemory_SetMemory(m_pMemory, 0x0, 0x11111111, READ_WRITE);
    SimpleMemory_SetMemory(m_pMemory, 0x4, 0x22222222, READ_WRITE);
    SimpleMemory_SetMemory(m_pMemory, 0x10000000, 0x33333333, READ_WRITE);
    SimpleMemory_SetMemory(m_pMemory, 0x10000004, 0x44444444, READ_WRITE);
    SimpleMemory_SetMemory(m_pMemory, 0x10000008, 0x55555555, READ_WRITE);

    CHECK_EQUAL(0x11111111, IMemory_Read32(m_pMemory, 0));
    CHECK_EQUAL(0x22222222, IMemory_Read32(m_pMemory, 4));
    CHECK_EQUAL(0x33333333, IMemory_Read32(m_pMemory, 0x10000000));
    CHECK_EQUAL(0x44444444, IMemory_Read32(m_pMemory, 0x10000004));
    CHECK_EQUAL(0x55555555, IMemory_Read32(m_pMemory, 0x10000008));
}

TEST(SimpleMemory, AttemptToRead16UnsetMemory)
{
    __try_and_catch(IMemory_Read16(m_pMemory, 0x10000000));
    CHECK_EQUAL(busErrorException, getExceptionCode());
}

TEST(SimpleMemory, ValidRead16Calls)
{
    SimpleMemory_SetMemory(m_pMemory, 0x10000000, 0x12345678, READ_WRITE);

    CHECK_EQUAL(0x5678, IMemory_Read16(m_pMemory, 0x10000000));
    CHECK_EQUAL(0x1234, IMemory_Read16(m_pMemory, 0x10000002));
}

TEST(SimpleMemory, AttemptToRead8UnsetMemory)
{
    __try_and_catch(IMemory_Read8(m_pMemory, 0x10000000));
    CHECK_EQUAL(busErrorException, getExceptionCode());
}

TEST(SimpleMemory, ValidRead8Calls)
{
    SimpleMemory_SetMemory(m_pMemory, 0x10000000, 0x12345678, READ_WRITE);

    CHECK_EQUAL(0x78, IMemory_Read8(m_pMemory, 0x10000000));
    CHECK_EQUAL(0x56, IMemory_Read8(m_pMemory, 0x10000001));
    CHECK_EQUAL(0x34, IMemory_Read8(m_pMemory, 0x10000002));
    CHECK_EQUAL(0x12, IMemory_Read8(m_pMemory, 0x10000003));
}

TEST(SimpleMemory, AttemptToWrite32UnsetMemory)
{
    __try_and_catch(IMemory_Write32(m_pMemory, 0x10000000, 0x12345678));
    CHECK_EQUAL(busErrorException, getExceptionCode());
}

TEST(SimpleMemory, AttemptToWrite32ReadOnlyMemory)
{
    SimpleMemory_SetMemory(m_pMemory, 0x10000000, 0xFFFFFFFF, READ_ONLY);
    __try_and_catch(IMemory_Write32(m_pMemory, 0x10000000, 0x12345678));
    CHECK_EQUAL(busErrorException, getExceptionCode());
}

TEST(SimpleMemory, ValidWrite32Calls)
{
    SimpleMemory_SetMemory(m_pMemory, 0x0, 0x11111111, READ_WRITE);
    SimpleMemory_SetMemory(m_pMemory, 0x4, 0x22222222, READ_WRITE);
    SimpleMemory_SetMemory(m_pMemory, 0x10000000, 0x33333333, READ_WRITE);
    SimpleMemory_SetMemory(m_pMemory, 0x10000004, 0x44444444, READ_WRITE);
    SimpleMemory_SetMemory(m_pMemory, 0x10000008, 0x55555555, READ_WRITE);

    IMemory_Write32(m_pMemory, 0x0, 0xFFFFFFFF);
    IMemory_Write32(m_pMemory, 0x4, 0xEEEEEEEE);
    IMemory_Write32(m_pMemory, 0x10000000, 0xDDDDDDDD);
    IMemory_Write32(m_pMemory, 0x10000004, 0xCCCCCCCC);
    IMemory_Write32(m_pMemory, 0x10000008, 0xBBBBBBBB);

    CHECK_EQUAL(0xFFFFFFFF, IMemory_Read32(m_pMemory, 0x00000000));
    CHECK_EQUAL(0xEEEEEEEE, IMemory_Read32(m_pMemory, 0x00000004));
    CHECK_EQUAL(0xDDDDDDDD, IMemory_Read32(m_pMemory, 0x10000000));
    CHECK_EQUAL(0xCCCCCCCC, IMemory_Read32(m_pMemory, 0x10000004));
    CHECK_EQUAL(0xBBBBBBBB, IMemory_Read32(m_pMemory, 0x10000008));
}

TEST(SimpleMemory, AttemptToWrite16UnsetMemory)
{
    __try_and_catch(IMemory_Write16(m_pMemory, 0x10000000, 0x1234));
    CHECK_EQUAL(busErrorException, getExceptionCode());
}

TEST(SimpleMemory, AttemptToWrite16ReadOnlyMemory)
{
    SimpleMemory_SetMemory(m_pMemory, 0x10000000, 0xFFFFFFFF, READ_ONLY);
    __try_and_catch(IMemory_Write16(m_pMemory, 0x10000000, 0x1234));
    CHECK_EQUAL(busErrorException, getExceptionCode());
}

TEST(SimpleMemory, ValidWrite16Calls)
{
    SimpleMemory_SetMemory(m_pMemory, 0x10000000, 0xFFFFFFFF, READ_WRITE);

    IMemory_Write16(m_pMemory, 0x10000000, 0x1234);
    IMemory_Write16(m_pMemory, 0x10000002, 0x5678);

    CHECK_EQUAL(0x56781234, IMemory_Read32(m_pMemory, 0x10000000));
}

TEST(SimpleMemory, AttemptToWrite8UnsetMemory)
{
    __try_and_catch(IMemory_Write8(m_pMemory, 0x10000000, 0x12));
    CHECK_EQUAL(busErrorException, getExceptionCode());
}

TEST(SimpleMemory, AttemptToWrite8ReadOnlyMemory)
{
    SimpleMemory_SetMemory(m_pMemory, 0x10000000, 0xFFFFFFFF, READ_ONLY);
    __try_and_catch(IMemory_Write8(m_pMemory, 0x10000000, 0x12));
    CHECK_EQUAL(busErrorException, getExceptionCode());
}

TEST(SimpleMemory, ValidWrite8Calls)
{
    SimpleMemory_SetMemory(m_pMemory, 0x10000000, 0xFFFFFFFF, READ_WRITE);

    IMemory_Write8(m_pMemory, 0x10000003, 0x78);
    IMemory_Write8(m_pMemory, 0x10000002, 0x56);
    IMemory_Write8(m_pMemory, 0x10000001, 0x34);
    IMemory_Write8(m_pMemory, 0x10000000, 0x12);

    CHECK_EQUAL(0x78563412, IMemory_Read32(m_pMemory, 0x10000000));
}

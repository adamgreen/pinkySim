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
#include <string.h>

extern "C"
{
#include <comm.h>
#include <try_catch.h>
}
#include <commMock.h>

// Include C++ headers for test harness.
#include "CppUTest/TestHarness.h"

TEST_GROUP(commMock)
{
    void setup()
    {
        clearExceptionCode();
    }

    void teardown()
    {
        LONGS_EQUAL( noException , getExceptionCode() );
        mockCommUninitTransmitDataBuffer();
        clearExceptionCode();
    }
};

TEST(commMock, commHasRecieveData_Empty)
{
    static const char emptyData[] = "";

    mockCommInitReceiveData(emptyData);
    CHECK_FALSE( commHasReceiveData() );
}

TEST(commMock, commHasRecieveData_NotEmpty)
{
    static const char testData[] = "$";

    mockCommInitReceiveData(testData);
    CHECK_TRUE( commHasReceiveData() );
}

TEST(commMock, commRecieveChar_NotEmpty)
{
    static const char testData[] = "$";

    mockCommInitReceiveData(testData);
    LONGS_EQUAL( '$', commReceiveChar() );
}

TEST(commMock, commHasReceiveData_SwitchToEmptyGdbPacket)
{
    static const char emptyData[] = "";
    static const char emptyGdbPacket[] = "$#00";
    char              buffer[16];
    char*             p = buffer;

    mockCommInitReceiveData(emptyData);
    CHECK_FALSE( commHasReceiveData() );
    CHECK_TRUE( commHasReceiveData() );

    while (commHasReceiveData())
    {
        *p++ = (char)commReceiveChar();
    }
    LONGS_EQUAL ( strlen(emptyGdbPacket), (p - buffer) );
    CHECK( 0 == memcmp(emptyGdbPacket, buffer, strlen(emptyGdbPacket)) );
}

TEST(commMock, commReceiveChar_SwitchToEmptyGdbPacket)
{
    static const char emptyData[] = "";
    static const char emptyGdbPacket[] = "$#00";
    char              buffer[16];
    char*             p = buffer;

    mockCommInitReceiveData(emptyData);

    do
    {
        *p++ = (char)commReceiveChar();
    }
    while (commHasReceiveData());

    LONGS_EQUAL ( strlen(emptyGdbPacket), (p - buffer) );
    CHECK( 0 == memcmp(emptyGdbPacket, buffer, strlen(emptyGdbPacket)) );
}

TEST(commMock, mockCommReceiveEmptyGdbPacket)
{
    static const char emptyGdbPacket[] = "$#00";
    char              buffer[16];
    char*             p = buffer;

    mockCommInitReceiveData(emptyGdbPacket);

    while (commHasReceiveData())
    {
        *p++ = (char)commReceiveChar();
    }
    LONGS_EQUAL ( strlen(emptyGdbPacket), (p - buffer) );
    CHECK( 0 == memcmp(emptyGdbPacket, buffer, strlen(emptyGdbPacket)) );
}

TEST(commMock, mockCommReceiveTwoGdbPacket)
{
    static const char packet1[] = "$packet1#00";
    static const char packet2[] = "$packet2#ff";
    char              buffer[16];
    char*             p = buffer;

    mockCommInitReceiveData(packet1, packet2);

    while (commHasReceiveData())
    {
        *p++ = (char)commReceiveChar();
    }
    LONGS_EQUAL ( strlen(packet1), (p - buffer) );
    CHECK( 0 == memcmp(packet1, buffer, strlen(packet1)) );

    p = buffer;
    while (commHasReceiveData())
    {
        *p++ = (char)commReceiveChar();
    }
    LONGS_EQUAL ( strlen(packet2), (p - buffer) );
    CHECK( 0 == memcmp(packet2, buffer, strlen(packet2)) );
}

TEST(commMock, TransmitAndCapture1Byte)
{
    mockCommInitTransmitDataBuffer(2);

    commSendChar('-');

    CHECK_TRUE( mockCommDoesTransmittedDataEqual("-") );
}

TEST(commMock, TransmitAndCapture2BytesWithOverflow)
{
    mockCommInitTransmitDataBuffer(2);

    commSendChar('-');
    commSendChar('+');
    commSendChar('*');

    CHECK_TRUE( mockCommDoesTransmittedDataEqual("-+") );
}

TEST(commMock, TransmitAndFailToCompareByLength)
{
    mockCommInitTransmitDataBuffer(2);

    commSendChar('-');

    CHECK_FALSE( mockCommDoesTransmittedDataEqual("-+") );
}

TEST(commMock, TransmitAndFailToCompareByData)
{
    mockCommInitTransmitDataBuffer(2);

    commSendChar('-');

    CHECK_FALSE( mockCommDoesTransmittedDataEqual("+") );
}

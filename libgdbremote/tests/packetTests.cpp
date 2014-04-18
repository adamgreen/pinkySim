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
#include "packet.h"
#include "try_catch.h"
}
#include "commMock.h"

// Include C++ headers for test harness.
#include "CppUTest/TestHarness.h"

TEST_GROUP(Packet)
{
    Packet            m_packet;
    Buffer            m_buffer;
    char*             m_pCharacterArray;
    int               m_exceptionThrown;
    static const char m_fillChar = 0xFF;
    
    void setup()
    {
        m_pCharacterArray = NULL;
        allocateBuffer(32);
        m_exceptionThrown = 0;
        mockCommInitTransmitDataBuffer(16);
    }

    void teardown()
    {
        validateNoExceptionThrown();
        mockCommUninitTransmitDataBuffer();
        free(m_pCharacterArray);
    }
    
    void validateNoExceptionThrown()
    {
        CHECK_FALSE ( m_exceptionThrown );
        LONGS_EQUAL ( 0, getExceptionCode() );
    }
    
    void allocateBuffer(size_t sizeOfBuffer)
    {
        free(m_pCharacterArray);
        m_pCharacterArray = (char*)malloc(sizeOfBuffer);
        memset(m_pCharacterArray, m_fillChar, sizeOfBuffer);
        bufferInit(&m_buffer, m_pCharacterArray, sizeOfBuffer);
    }
    
    void allocateBuffer(const char* pBufferString)
    {
        free(m_pCharacterArray);
        m_pCharacterArray = NULL;
        bufferInit(&m_buffer, (char*)pBufferString, strlen(pBufferString));
    }

    void tryPacketGet()
    {
        __try
            packetGet(&m_packet, &m_buffer);
        __catch
            m_exceptionThrown = 1;
    }
    
    void tryPacketSend()
    {
        __try
            packetSend(&m_packet, &m_buffer);
        __catch
            m_exceptionThrown = 1;
    }
    
    void validateThatEmptyGdbPacketWasRead()
    {
        LONGS_EQUAL( 0, bufferGetLength(&m_buffer) );
    }
    
    void validateBufferMatches(const char* pExpectedOutput)
    {
        CHECK_TRUE( bufferMatchesString(&m_buffer, pExpectedOutput, strlen(pExpectedOutput)) );
    }
};

TEST(Packet, PacketGetFromGDB_Empty)
{
    mockCommInitReceiveData("$#00");
    tryPacketGet();
    validateThatEmptyGdbPacketWasRead();
    CHECK_TRUE( mockCommDoesTransmittedDataEqual("+") );
}

TEST(Packet, PacketGetFromGDB_Short)
{
    mockCommInitReceiveData("$?#3f");
    tryPacketGet();
    validateBufferMatches("?");
    CHECK_TRUE( mockCommDoesTransmittedDataEqual("+") );
}

TEST(Packet, PacketGetFromGDB_ShortWithUpperCaseHexDigits)
{
    mockCommInitReceiveData("$?#3F");
    tryPacketGet();
    validateBufferMatches("?");
    CHECK_TRUE( mockCommDoesTransmittedDataEqual("+") );
}

TEST(Packet, PacketGetFromGDB_BadChecksum)
{
    mockCommInitReceiveData("$?#f3");
    tryPacketGet();
    validateThatEmptyGdbPacketWasRead();
    CHECK_TRUE( mockCommDoesTransmittedDataEqual("-+") ); 
}

TEST(Packet, PacketGetFromGDB_BadHexDigitInChecksum)
{
    mockCommInitReceiveData("$?#g3");
    tryPacketGet();
    validateThatEmptyGdbPacketWasRead();
    CHECK_TRUE( mockCommDoesTransmittedDataEqual("-+") ); 
}

TEST(Packet, PacketGetFromGDB_TwoPackets)
{
    mockCommInitReceiveData("$#00$?#3f");
    tryPacketGet();
    validateBufferMatches("?");
    CHECK_TRUE( mockCommDoesTransmittedDataEqual("+") );
}

TEST(Packet, PacketGetFromGDB_SearchForStartOfPacket)
{
    mockCommInitReceiveData("#00$?#3f");
    tryPacketGet();
    validateBufferMatches("?");
    CHECK_TRUE( mockCommDoesTransmittedDataEqual("+") );
}

TEST(Packet, PacketGetFromGDB_StartOfPacketWithinPacket)
{
    mockCommInitReceiveData("$?$?#3f");
    tryPacketGet();
    validateBufferMatches("?");
    CHECK_TRUE( mockCommDoesTransmittedDataEqual("+") );
}

TEST(Packet, PacketGetFromGDB_BufferTooSmall)
{
    mockCommInitReceiveData("$qSupported:qRelocInsn+#9af");
    allocateBuffer(8);
    tryPacketGet();
    validateThatEmptyGdbPacketWasRead();
    CHECK_TRUE( mockCommDoesTransmittedDataEqual("+") );
}

TEST(Packet, PacketSendToGDB_EmptyWithAck)
{
    allocateBuffer("");
    mockCommInitReceiveData("+");
    tryPacketSend();
    CHECK_TRUE( mockCommDoesTransmittedDataEqual("$#00") );
}

TEST(Packet, PacketSendToGDB_OkWithAck)
{
    allocateBuffer("OK");
    mockCommInitReceiveData("+");
    tryPacketSend();
    CHECK_TRUE( mockCommDoesTransmittedDataEqual("$OK#9a") );
}

TEST(Packet, PacketSendToGDB_OkWithNackAndAck)
{
    allocateBuffer("OK");
    mockCommInitReceiveData("-+");
    tryPacketSend();
    CHECK_TRUE( mockCommDoesTransmittedDataEqual("$OK#9a$OK#9a") );
}

TEST(Packet, PacketSendToGDB_OkWithCancelForNewPacket)
{
    allocateBuffer("OK");
    mockCommInitReceiveData("$");
    tryPacketSend();
    CHECK_TRUE( mockCommDoesTransmittedDataEqual("$OK#9a") );
}
   

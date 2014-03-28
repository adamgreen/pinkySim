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
/* 'Class' to manage the sending and receiving of packets to/from gdb.  Takes care of crc and ack/nak handling too. */
#include <string.h>
#include <stdint.h>
#include <comm.h>
#include <packet.h>
#include "hexConvert.h"


void packetInit(Packet* pPacket)
{
    memset(pPacket, 0, sizeof(*pPacket));
}


static void rememberBufferToUseForPacketData(Packet* pPacket, Buffer* pBuffer);
static void getMostRecentPacket(Packet* pPacket);
static void getPacketDataAndExpectedChecksum(Packet* pPacket);
static void waitForStartOfNextPacket(Packet* pPacket);
static char getNextCharFromGdb(Packet* pPacket);
static int  getPacketData(Packet* pPacket);
static void clearChecksum(Packet* pPacket);
static void updateChecksum(Packet* pPacket, char nextChar);
static void extractExpectedChecksum(Packet* pPacket);
static int  isChecksumValid(Packet* pPacket);
static void sendACKToGDB(void);
static void sendNAKToGDB(void);
static void resetBufferToEnableFutureReadingOfValidPacketData(Packet* pPacket);
void packetGet(Packet* pPacket, Buffer* pBuffer)
{
    rememberBufferToUseForPacketData(pPacket, pBuffer);
    do
    {
        getMostRecentPacket(pPacket);
    } while(!isChecksumValid(pPacket));
    
    resetBufferToEnableFutureReadingOfValidPacketData(pPacket);
}

static void rememberBufferToUseForPacketData(Packet* pPacket, Buffer* pBuffer)
{
    pPacket->pBuffer = pBuffer;
}

static void getMostRecentPacket(Packet* pPacket)
{
    do
    {
        getPacketDataAndExpectedChecksum(pPacket);
    } while (commHasReceiveData());
    
    if (!isChecksumValid(pPacket))
    {
        sendNAKToGDB();
        return;
    }

    sendACKToGDB();
}

static void getPacketDataAndExpectedChecksum(Packet* pPacket)
{
    int completePacket;
    
    do
    {
        waitForStartOfNextPacket(pPacket);
        completePacket = getPacketData(pPacket);
    } while (!completePacket);
    
    extractExpectedChecksum(pPacket);
}

static void waitForStartOfNextPacket(Packet* pPacket)
{
    char nextChar = pPacket->lastChar;
    
    /* Wait for the packet start character, '$', and ignore all other characters. */
    while (nextChar != '$')
        nextChar = getNextCharFromGdb(pPacket);
}

static char getNextCharFromGdb(Packet* pPacket)
{
    char nextChar = commReceiveChar();
    pPacket->lastChar = nextChar;
    return nextChar;
}

static int getPacketData(Packet* pPacket)
{
    char nextChar;
    
    bufferReset(pPacket->pBuffer);
    clearChecksum(pPacket);
    nextChar = getNextCharFromGdb(pPacket);
    while (bufferBytesLeft(pPacket->pBuffer) > 0 && nextChar != '$' && nextChar != '#')
    {
        updateChecksum(pPacket, nextChar);
        bufferWriteChar(pPacket->pBuffer, nextChar);
        nextChar = getNextCharFromGdb(pPacket);
    }
    
    /* Return success if the expected end of packet character, '#', was received. */
    return (nextChar == '#');
}

static void clearChecksum(Packet* pPacket)
{
    pPacket->calculatedChecksum = 0;
}

static void updateChecksum(Packet* pPacket, char nextChar)
{
    pPacket->calculatedChecksum += (unsigned char)nextChar;
}

static void extractExpectedChecksum(Packet* pPacket)
{
    unsigned char expectedChecksumHiNibble = HexCharToNibble(getNextCharFromGdb(pPacket));
    unsigned char expectedChecksumLoNibble = HexCharToNibble(getNextCharFromGdb(pPacket));

    pPacket->expectedChecksum = (expectedChecksumHiNibble << 4) | expectedChecksumLoNibble;
}

static int isChecksumValid(Packet* pPacket)
{
    return (pPacket->expectedChecksum == pPacket->calculatedChecksum);
}

static void sendACKToGDB(void)
{
    commSendChar('+');
}

static void sendNAKToGDB(void)
{
    commSendChar('-');
}

static void resetBufferToEnableFutureReadingOfValidPacketData(Packet* pPacket)
{
    bufferSetEndOfBuffer(pPacket->pBuffer);
    bufferReset(pPacket->pBuffer);
}


static void sendPacket(Packet* pPacket);
static void sendPacketHeaderByte(void);
static void sendPacketData(Packet* pPacket);
static void sendPacketChecksum(Packet* pPacket);
static void sendByteAsHex(unsigned char byte);
static int  receiveCharAfterSkippingControlC(Packet* pPacket);
void packetSend(Packet* pPacket, Buffer* pBuffer)
{
    char  charFromGdb;
    
    /* Keeps looping until GDB sends back the '+' packet acknowledge character.  If GDB sends a '$' then it is trying
       to send a packet so cancel this send attempt. */
    rememberBufferToUseForPacketData(pPacket, pBuffer);
    do
    {
        sendPacket(pPacket);
        charFromGdb = receiveCharAfterSkippingControlC(pPacket);
    } while (charFromGdb != '+' && charFromGdb != '$');
}

static void sendPacket(Packet* pPacket)
{
    /* Send packet of format: "$<DataInHex>#<1ByteChecksumInHex> */
    bufferReset(pPacket->pBuffer);
    clearChecksum(pPacket);

    sendPacketHeaderByte();
    sendPacketData(pPacket);
    sendPacketChecksum(pPacket);
}

static void sendPacketHeaderByte(void)
{
    commSendChar('$');
}

static void sendPacketData(Packet* pPacket)
{
    while (bufferBytesLeft(pPacket->pBuffer) > 0)
    {
        char currChar = bufferReadChar(pPacket->pBuffer);
        commSendChar(currChar);
        updateChecksum(pPacket, currChar);
    }
}

static void sendPacketChecksum(Packet* pPacket)
{
    commSendChar('#');
    sendByteAsHex(pPacket->calculatedChecksum);
}

static void sendByteAsHex(unsigned char byte)
{
    commSendChar(NibbleToHexChar[EXTRACT_HI_NIBBLE(byte)]);
    commSendChar(NibbleToHexChar[EXTRACT_LO_NIBBLE(byte)]);
}

static int receiveCharAfterSkippingControlC(Packet* pPacket)
{
    static const int controlC = 0x03;
    int              nextChar;
    
    do
    {
        nextChar = getNextCharFromGdb(pPacket);
    }
    while (nextChar == controlC);
    
    return nextChar;
}

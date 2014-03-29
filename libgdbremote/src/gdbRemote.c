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
#include <gdbRemote.h>
#include <packet.h>


static void sendGetRegistersCommand(void);
static void receiveAndParseByteArrayResponse(uint8_t* pReadBuffer, uint8_t readSize);
static void sendSetRegisterCommand(const PinkySimContext* pContext);
static void sendByteArrayAsHex(Buffer* pBuffer, const uint8_t* pWriteBuffer, uint8_t writeSize);
static void verifyOkResponse(void);
static void sendReadMemoryCommand(uint32_t address, uint32_t readSize);
static void sendWriteMemoryCommand(uint32_t address, const void* pWriteBuffer, uint32_t writeSize);
static void sendSingleStepCommand(void);
static void verifyTResponse(void);


void gdbRemoteGetRegisters(PinkySimContext* pContext)
{
    sendGetRegistersCommand();
    receiveAndParseByteArrayResponse((uint8_t*)&pContext->R[0], 17 * sizeof(uint32_t));
}

static void sendGetRegistersCommand(void)
{
    Buffer   buffer;
    Packet   packet;
    char     data[512];
    
    bufferInit(&buffer, data, sizeof(data));
    bufferWriteString(&buffer, "g");
    bufferSetEndOfBuffer(&buffer);

    packetInit(&packet);
    packetSend(&packet, &buffer);
}

static void receiveAndParseByteArrayResponse(uint8_t* pReadBuffer, uint8_t readSize)
{
    uint8_t* pCurr;
    uint8_t* pEnd = pReadBuffer + readSize;
    Buffer   buffer;
    Packet   packet;
    char     data[512];

    bufferInit(&buffer, data, sizeof(data));
    packetInit(&packet);
    packetGet(&packet, &buffer);

    for (pCurr = pReadBuffer ; pCurr < pEnd ; pCurr++)
        *pCurr = bufferReadByteAsHex(&buffer);
}



void gdbRemoteSetRegisters(const PinkySimContext* pContext)
{
    sendSetRegisterCommand(pContext);
    verifyOkResponse();
}

static void sendSetRegisterCommand(const PinkySimContext* pContext)
{
    Buffer   buffer;
    Packet   packet;
    char     data[512];

    bufferInit(&buffer, data, sizeof(data));
    bufferWriteChar(&buffer, 'G');
    sendByteArrayAsHex(&buffer, (uint8_t*)&pContext->R[0], 17 * sizeof(uint32_t));
    bufferSetEndOfBuffer(&buffer);

    packetInit(&packet);
    packetSend(&packet, &buffer);
}

static void sendByteArrayAsHex(Buffer* pBuffer, const uint8_t* pWriteBuffer, uint8_t writeSize)
{
    const uint8_t* pEnd = pWriteBuffer + writeSize;
    const uint8_t* pCurr;

    for (pCurr = pWriteBuffer ; pCurr < pEnd ; pCurr++)
        bufferWriteByteAsHex(pBuffer, *pCurr);
}

static void verifyOkResponse(void)
{
    static const char expectedResponse[] = "OK";
    Buffer            buffer;
    Packet            packet;
    char              data[512];

    bufferInit(&buffer, data, sizeof(data));
    packetInit(&packet);
    packetGet(&packet, &buffer);
    if (!bufferMatchesString(&buffer, expectedResponse, sizeof(expectedResponse)-1))
        __throw(badResponseException);
}



void gdbRemoteReadMemory(uint32_t address, void* pvReadBuffer, uint32_t readSize)
{
    sendReadMemoryCommand(address, readSize);
    receiveAndParseByteArrayResponse((uint8_t*)pvReadBuffer, readSize);
}

static void sendReadMemoryCommand(uint32_t address, uint32_t readSize)
{
    Buffer   buffer;
    Packet   packet;
    char     data[512];
    
    bufferInit(&buffer, data, sizeof(data));
    bufferWriteChar(&buffer, 'm');
    bufferWriteUIntegerAsHex(&buffer, address);
    bufferWriteChar(&buffer, ',');
    bufferWriteUIntegerAsHex(&buffer, readSize);
    bufferSetEndOfBuffer(&buffer);

    packetInit(&packet);
    packetSend(&packet, &buffer);
}



void gdbRemoteWriteMemory(uint32_t address, const void* pWriteBuffer, uint32_t writeSize)
{
    sendWriteMemoryCommand(address, pWriteBuffer, writeSize);
    verifyOkResponse();
}

static void sendWriteMemoryCommand(uint32_t address, const void* pWriteBuffer, uint32_t writeSize)
{
    Buffer   buffer;
    Packet   packet;
    char     data[512];

    bufferInit(&buffer, data, sizeof(data));
    bufferWriteChar(&buffer, 'M');
    bufferWriteUIntegerAsHex(&buffer, address);
    bufferWriteChar(&buffer, ',');
    bufferWriteUIntegerAsHex(&buffer, writeSize);
    bufferWriteChar(&buffer, ':');
    sendByteArrayAsHex(&buffer, (uint8_t*)pWriteBuffer, writeSize);
    bufferSetEndOfBuffer(&buffer);

    packetInit(&packet);
    packetSend(&packet, &buffer);
}



void gdbRemoteSingleStep(void)
{
    sendSingleStepCommand();
    verifyTResponse();
}

static void sendSingleStepCommand(void)
{
    Buffer   buffer;
    Packet   packet;
    char     data[512];
    
    bufferInit(&buffer, data, sizeof(data));
    bufferWriteString(&buffer, "s");
    bufferSetEndOfBuffer(&buffer);

    packetInit(&packet);
    packetSend(&packet, &buffer);
}

static void verifyTResponse(void)
{
    Buffer            buffer;
    Packet            packet;
    char              data[512];

    bufferInit(&buffer, data, sizeof(data));
    packetInit(&packet);
    packetGet(&packet, &buffer);
    if (!bufferIsNextCharEqualTo(&buffer, 'T'))
        __throw(badResponseException);
}

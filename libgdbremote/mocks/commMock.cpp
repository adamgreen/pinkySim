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
#include <buffer.h>
#include <try_catch.h>
}
#include "commMock.h"

static const char  g_emptyPacket[] = "$#00";
static Buffer      g_receiveBuffer1;
static Buffer      g_receiveBuffer2;
static Buffer*     g_pReceiveBuffer;
static char*       g_pTransmitDataBufferStart;
static char*       g_pTransmitDataBufferEnd;
static char*       g_pTransmitDataBufferCurr;


static uint32_t isReceiveBufferEmpty();
static void     waitForReceiveData();
static size_t   getTransmitDataBufferSize();


void mockCommInitReceiveData(const char* pDataToReceive)
{
    bufferInit(&g_receiveBuffer1, (char*)pDataToReceive, strlen(pDataToReceive));
    bufferInit(&g_receiveBuffer2, (char*)g_emptyPacket, strlen(g_emptyPacket));
    g_pReceiveBuffer = &g_receiveBuffer1;
}

void mockCommInitReceiveData(const char* pDataToReceive1, const char* pDataToReceive2)
{
    bufferInit(&g_receiveBuffer1, (char*)pDataToReceive1, strlen(pDataToReceive1));
    bufferInit(&g_receiveBuffer2, (char*)pDataToReceive2, strlen(pDataToReceive2));
    g_pReceiveBuffer = &g_receiveBuffer1;
}

void mockCommInitTransmitDataBuffer(size_t Size)
{
    mockCommUninitTransmitDataBuffer();
    g_pTransmitDataBufferStart = (char*)malloc(Size);
    g_pTransmitDataBufferCurr = g_pTransmitDataBufferStart;
    g_pTransmitDataBufferEnd = g_pTransmitDataBufferStart + Size;
}

void mockCommUninitTransmitDataBuffer()
{
    free(g_pTransmitDataBufferStart);
    g_pTransmitDataBufferStart = NULL;
    g_pTransmitDataBufferCurr = NULL;
    g_pTransmitDataBufferEnd = NULL;
}

int mockCommDoesTransmittedDataEqual(const char* thisString)
{
    size_t stringLength = strlen(thisString);

    if (getTransmitDataBufferSize() != stringLength)
        return 0;
    return !memcmp(thisString, g_pTransmitDataBufferStart, stringLength);
}



// Test mocks of the communication APIs
uint32_t commHasReceiveData(void)
{
    if (isReceiveBufferEmpty())
    {
        g_pReceiveBuffer = &g_receiveBuffer2;
        return 0;
    }
    
    return 1;
}

int commReceiveChar(void)
{
    waitForReceiveData();

    int character = bufferReadChar(g_pReceiveBuffer);

    clearExceptionCode();

    return character;
}

void commSendChar(int character)
{
    if (g_pTransmitDataBufferCurr < g_pTransmitDataBufferEnd)
        *g_pTransmitDataBufferCurr++ = (char)character;
}

static uint32_t isReceiveBufferEmpty()
{
    return (uint32_t)(bufferBytesLeft(g_pReceiveBuffer) == 0);
}

static void waitForReceiveData()
{
    while (!commHasReceiveData())
    {
    }
}

static size_t getTransmitDataBufferSize()
{
    return g_pTransmitDataBufferCurr - g_pTransmitDataBufferStart;
}

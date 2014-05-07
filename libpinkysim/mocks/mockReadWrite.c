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
#include <errno.h>
#include <string.h>
#include <unistd.h>

static ssize_t mock_read(int fildes, void *buf, size_t nbyte);
static ssize_t mock_write(int fildes, const void *buf, size_t nbyte);


ssize_t (*hook_read)(int fildes, void *buf, size_t nbyte) = mock_read;
ssize_t (*hook_write)(int fildes, const void *buf, size_t nbyte) = mock_write;


static       int      g_readResult;
static       int      g_readError;
static const uint8_t* g_pReadStart;
static const uint8_t* g_pReadEnd;
static const uint8_t* g_pReadCurr;
static char*          g_pStdOutStart;
static char*          g_pStdOutEnd;
static char*          g_pStdOutCurr;
static char*          g_pStdErrStart;
static char*          g_pStdErrEnd;
static char*          g_pStdErrCurr;


static ssize_t mock_read(int fildes, void *buf, size_t nbyte)
{
    int bytesLeft = g_pReadEnd - g_pReadCurr;
    if (fildes != STDIN_FILENO)
    {
        errno = EBADF;
        return -1;
    }

    if (g_readResult)
    {
        errno = g_readError;
        return g_readResult;
    }

    if (nbyte > (size_t)bytesLeft)
        nbyte = bytesLeft;
    memcpy(buf, g_pReadCurr, nbyte);
    g_pReadCurr += nbyte;
    return nbyte;
}


static ssize_t mock_write(int fildes, const void *buf, size_t nbyte)
{
    char*  pBuffer;
    char** ppCurr;
    int    bytesLeft;

    switch (fildes)
    {
    case STDOUT_FILENO:
        pBuffer = g_pStdOutCurr;
        ppCurr = &g_pStdOutCurr;
        bytesLeft = g_pStdOutEnd - g_pStdOutCurr;
        break;
    case STDERR_FILENO:
        pBuffer = g_pStdErrCurr;
        ppCurr = &g_pStdErrCurr;
        bytesLeft = g_pStdErrEnd - g_pStdErrCurr;
        break;
    default:
        errno = EBADF;
        return -1;
    }

    nbyte = nbyte > (size_t)bytesLeft ? bytesLeft : nbyte;
    memcpy(pBuffer, buf, nbyte);
    (*ppCurr) += nbyte;
    return nbyte;
}


void mockReadWrite_SetReadData(const void* pvData, size_t dataSize)
{
    g_pReadStart = g_pReadCurr = (uint8_t*)pvData;
    g_pReadEnd = g_pReadStart + dataSize;
}

void mockReadWrite_SetReadToFail(int result, int err)
{
    g_readResult = result;
    g_readError = err;
}

void mockReadWrite_CreateWriteBuffer(size_t bufferSize)
{
    g_pStdOutStart = malloc(bufferSize + 1);
    g_pStdOutEnd = g_pStdOutStart + bufferSize;
    g_pStdOutCurr = g_pStdOutStart;
    g_pStdErrStart = malloc(bufferSize + 1);
    g_pStdErrEnd = g_pStdErrStart + bufferSize;
    g_pStdErrCurr = g_pStdErrStart;
}

const char* mockReadWrite_GetStdOutData(void)
{
    *g_pStdOutCurr = '\0';
    return g_pStdOutStart;
}

const char* mockReadWrite_GetStdErrData(void)
{
    *g_pStdErrCurr = '\0';
    return g_pStdErrStart;
}

void mockReadWrite_Uninit(void)
{
    g_pReadStart = g_pReadCurr = g_pReadEnd = NULL;
    free(g_pStdOutStart);
    g_pStdOutStart = g_pStdOutCurr = g_pStdOutEnd = NULL;
    free(g_pStdErrStart);
    g_pStdErrStart = g_pStdErrCurr = g_pStdErrEnd = NULL;
    g_readResult = 0;
}

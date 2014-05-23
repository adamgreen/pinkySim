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

static int     mock_open(const char *path, int oflag, ...);
static ssize_t mock_read(int fildes, void *buf, size_t nbyte);
static ssize_t mock_write(int fildes, const void *buf, size_t nbyte);
static off_t   mock_lseek(int fildes, off_t offset, int whence);
static int     mock_close(int fildes);
static int     mock_unlink(const char *path);
static int     mock_rename(const char *oldPath, const char *newPath);


int     (*hook_open)(const char *path, int oflag, ...) = mock_open;
ssize_t (*hook_read)(int fildes, void *buf, size_t nbyte) = mock_read;
ssize_t (*hook_write)(int fildes, const void *buf, size_t nbyte) = mock_write;
off_t   (*hook_lseek)(int fildes, off_t offset, int whence) = mock_lseek;
int     (*hook_close)(int fildes) = mock_close;
int     (*hook_unlink)(const char *path) = mock_unlink;
int     (*hook_rename)(const char *oldPath, const char *newPath) = mock_rename;


typedef struct CallResult
{
    int result;
    int error;
} CallResult;

#define CALL_GLOBAL(X) \
   CallResult g_##X;

#define CALL_MOCK(X) \
    if (g_##X.result < 0) \
        errno = g_##X.error; \
    return g_##X.result;

#define CALL_SET(X) \
    g_##X.result = result; \
    g_##X.error = err;


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
CALL_GLOBAL(open)
CALL_GLOBAL(lseek)
CALL_GLOBAL(close)
CALL_GLOBAL(unlink)
CALL_GLOBAL(rename)


static int mock_open(const char *path, int oflag, ...)
{
    CALL_MOCK(open)
}

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

static off_t mock_lseek(int fildes, off_t offset, int whence)
{
    CALL_MOCK(lseek)
}

static int mock_close(int fildes)
{
    CALL_MOCK(close)
}

static int mock_unlink(const char *path)
{
    CALL_MOCK(unlink)
}

static int mock_rename(const char *oldPath, const char *newPath)
{
    CALL_MOCK(rename)
}


void mockFileIo_SetOpenToFail(int result, int err)
{
    CALL_SET(open);
}

void mockFileIo_SetReadData(const void* pvData, size_t dataSize)
{
    g_pReadStart = g_pReadCurr = (uint8_t*)pvData;
    g_pReadEnd = g_pReadStart + dataSize;
}

void mockFileIo_SetReadToFail(int result, int err)
{
    g_readResult = result;
    g_readError = err;
}

void mockFileIo_CreateWriteBuffer(size_t bufferSize)
{
    g_pStdOutStart = malloc(bufferSize + 1);
    g_pStdOutEnd = g_pStdOutStart + bufferSize;
    g_pStdOutCurr = g_pStdOutStart;
    g_pStdErrStart = malloc(bufferSize + 1);
    g_pStdErrEnd = g_pStdErrStart + bufferSize;
    g_pStdErrCurr = g_pStdErrStart;
}

const char* mockFileIo_GetStdOutData(void)
{
    *g_pStdOutCurr = '\0';
    return g_pStdOutStart;
}

const char* mockFileIo_GetStdErrData(void)
{
    *g_pStdErrCurr = '\0';
    return g_pStdErrStart;
}

void mockFileIo_SetLSeekToFail(int result, int err)
{
    CALL_SET(lseek);
}

void mockFileIo_SetCloseToFail(int result, int err)
{
    CALL_SET(close);
}

void mockFileIo_SetUnlinkToFail(int result, int err)
{
    CALL_SET(unlink);
}

void mockFileIo_SetRenameToFail(int result, int err)
{
    CALL_SET(rename);
}

void mockFileIo_Uninit(void)
{
    g_pReadStart = g_pReadCurr = g_pReadEnd = NULL;
    free(g_pStdOutStart);
    g_pStdOutStart = g_pStdOutCurr = g_pStdOutEnd = NULL;
    free(g_pStdErrStart);
    g_pStdErrStart = g_pStdErrCurr = g_pStdErrEnd = NULL;
    g_readResult = 0;
    mockFileIo_SetCloseToFail(0, 0);
}

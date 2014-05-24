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
/* Semihost functionality for redirecting to the GDB console. */
#include <cmd_file.h>
#include <core.h>
#include <errno.h>
#include <MallocFailureInject.h>
#include <MemorySim.h>
#include <mockFileIo.h>
#include <mri.h>
#include <mri4sim.h>
#include <NewlibSemihost.h>
#include <platforms.h>
#include <semihost.h>
#include <string.h>
#include <unistd.h>


static int isConsoleOutput(uint32_t fileDescriptor);
static void writeToFile(PlatformSemihostParameters* pSemihostParameters);
static int isGdbConnected(void);
static int isConsoleInput(uint32_t fileDescriptor);
static void readFromFile(PlatformSemihostParameters* pSemihostParameters);
static void copyHostStatToCommonStat(CommonStat* pTarget, const struct stat* pHost);


int handleNewlibSemihostWriteRequest(PlatformSemihostParameters* pSemihostParameters)
{
    TransferParameters parameters;

    writeToFile(pSemihostParameters);
    if (isConsoleOutput(pSemihostParameters->parameter1) && isGdbConnected())
    {
        parameters.fileDescriptor = pSemihostParameters->parameter1;
        parameters.bufferAddress = pSemihostParameters->parameter2;
        parameters.bufferSize = pSemihostParameters->parameter3;
        return IssueGdbFileWriteRequest(&parameters);
    }
    FlagSemihostCallAsHandled();
    return 1;
}

static int isConsoleOutput(uint32_t fileDescriptor)
{
    return fileDescriptor == STDOUT_FILENO || fileDescriptor == STDERR_FILENO;
}

static int isGdbConnected(void)
{
    return !Platform_CommIsWaitingForGdbToConnect();
}

static void writeToFile(PlatformSemihostParameters* pSemihostParameters)
{
    int32_t  file = pSemihostParameters->parameter1;
    uint32_t address = pSemihostParameters->parameter2;
    uint32_t size = pSemihostParameters->parameter3;

    __try
    {
        const void* pBuffer = MemorySim_MapSimulatedAddressToHostAddressForRead(mri4simGetContext()->pMemory, address, size);
        int writeResult = write(file, pBuffer, size);
        SetSemihostReturnValues(writeResult, errno);
    }
    __catch
    {
        SetSemihostReturnValues(-1, EFAULT);
    }
}

int handleNewlibSemihostReadRequest(PlatformSemihostParameters* pSemihostParameters)
{
    TransferParameters parameters;

    if (isConsoleInput(pSemihostParameters->parameter1)  && isGdbConnected())
    {
        parameters.fileDescriptor = pSemihostParameters->parameter1;
        parameters.bufferAddress = pSemihostParameters->parameter2;
        parameters.bufferSize = pSemihostParameters->parameter3;
        return IssueGdbFileReadRequest(&parameters);
    }
    readFromFile(pSemihostParameters);
    FlagSemihostCallAsHandled();
    return 1;
}

static int isConsoleInput(uint32_t fileDescriptor)
{
    return fileDescriptor == STDIN_FILENO;
}

static void readFromFile(PlatformSemihostParameters* pSemihostParameters)
{
    int32_t  file = pSemihostParameters->parameter1;
    uint32_t address = pSemihostParameters->parameter2;
    uint32_t size = pSemihostParameters->parameter3;

    __try
    {
        void* pBuffer = MemorySim_MapSimulatedAddressToHostAddressForWrite(mri4simGetContext()->pMemory, address, size);
        ssize_t readResult = read(file, pBuffer, size);
        SetSemihostReturnValues(readResult, errno);
    }
    __catch
    {
        SetSemihostReturnValues(-1, EFAULT);
    }
}

int handleNewlibSemihostOpenRequest(PlatformSemihostParameters* pSemihostParameters)
{
    uint32_t filenameAddress = pSemihostParameters->parameter1;
    uint32_t filenameLength = pSemihostParameters->parameter4;
    uint32_t flags = pSemihostParameters->parameter2;
    uint32_t mode = pSemihostParameters->parameter3;
    __try
    {
        const void* pFilename = MemorySim_MapSimulatedAddressToHostAddressForRead(mri4simGetContext()->pMemory,
                                                                                  filenameAddress,
                                                                                  filenameLength);

        int openResult = open(pFilename, flags, mode);
        SetSemihostReturnValues(openResult, errno);
    }
    __catch
    {
        SetSemihostReturnValues(-1, EFAULT);
    }
    FlagSemihostCallAsHandled();
    return 1;
}

int handleNewlibSemihostUnlinkRequest(PlatformSemihostParameters* pSemihostParameters)
{
    uint32_t filenameAddress = pSemihostParameters->parameter1;
    uint32_t filenameLength = pSemihostParameters->parameter2;

    __try
    {
        const void* pFilename = MemorySim_MapSimulatedAddressToHostAddressForRead(mri4simGetContext()->pMemory,
                                                                                  filenameAddress, filenameLength);
        int unlinkResult = unlink(pFilename);
        SetSemihostReturnValues(unlinkResult, errno);
    }
    __catch
    {
        SetSemihostReturnValues(-1, EFAULT);
    }

    FlagSemihostCallAsHandled();
    return 1;
}

int handleNewlibSemihostLSeekRequest(PlatformSemihostParameters* pSemihostParameters)
{
    uint32_t fileDescriptor = pSemihostParameters->parameter1;
    uint32_t offset = pSemihostParameters->parameter2;
    uint32_t whence = pSemihostParameters->parameter3;

    int lseekResult = lseek(fileDescriptor, offset, whence);
    SetSemihostReturnValues(lseekResult, errno);
    FlagSemihostCallAsHandled();
    return 1;
}

int handleNewlibSemihostCloseRequest(PlatformSemihostParameters* pSemihostParameters)
{
    uint32_t fileDescriptor = pSemihostParameters->parameter1;

    int closeResult = close(fileDescriptor);
    SetSemihostReturnValues(closeResult, errno);
    FlagSemihostCallAsHandled();
    return 1;
}

int handleNewlibSemihostFStatRequest(PlatformSemihostParameters* pSemihostParameters)
{
    uint32_t file = pSemihostParameters->parameter1;
    uint32_t fileStatAddress = pSemihostParameters->parameter2;

    __try
    {
        struct stat hostStat;
        CommonStat* pTargetStat = MemorySim_MapSimulatedAddressToHostAddressForWrite(mri4simGetContext()->pMemory,
                                                                                     fileStatAddress,
                                                                                     sizeof(*pTargetStat));
        int fstatResult = fstat(file, &hostStat);
        copyHostStatToCommonStat(pTargetStat, &hostStat);
        SetSemihostReturnValues(fstatResult, errno);
    }
    __catch
    {
        SetSemihostReturnValues(-1, EFAULT);
    }
    FlagSemihostCallAsHandled();
    return 1;
}

int handleNewlibSemihostStatRequest(PlatformSemihostParameters* pSemihostParameters)
{
    uint32_t filenameAddress = pSemihostParameters->parameter1;
    uint32_t filenameLength = pSemihostParameters->parameter3;
    uint32_t fileStatAddress = pSemihostParameters->parameter2;

    __try
    {
        struct stat hostStat;
        const void* pFilename = MemorySim_MapSimulatedAddressToHostAddressForRead(mri4simGetContext()->pMemory,
                                                                                  filenameAddress, filenameLength);
        CommonStat* pTargetStat = MemorySim_MapSimulatedAddressToHostAddressForWrite(mri4simGetContext()->pMemory,
                                                                                     fileStatAddress,
                                                                                     sizeof(*pTargetStat));
        int statResult = hook_stat(pFilename, &hostStat);
        copyHostStatToCommonStat(pTargetStat, &hostStat);
        SetSemihostReturnValues(statResult, errno);
    }
    __catch
    {
        SetSemihostReturnValues(-1, EFAULT);
    }
    FlagSemihostCallAsHandled();
    return 1;
}

static void copyHostStatToCommonStat(CommonStat* pTarget, const struct stat* pHost)
{
    pTarget->mode = pHost->st_mode;
    pTarget->size = pHost->st_size;
    pTarget->blocks = pHost->st_blocks;
    pTarget->blksize = pHost->st_blksize;
}

int handleNewlibSemihostRenameRequest(PlatformSemihostParameters* pSemihostParameters)
{
    uint32_t origFilenameAddress = pSemihostParameters->parameter1;
    uint32_t origFilenameLength = pSemihostParameters->parameter3;
    uint32_t newFilenameAddress = pSemihostParameters->parameter2;
    uint32_t newFilenameLength = pSemihostParameters->parameter4;

    __try
    {
        const void* pOrigFilename = MemorySim_MapSimulatedAddressToHostAddressForRead(mri4simGetContext()->pMemory,
                                                                                      origFilenameAddress,
                                                                                      origFilenameLength);
        const void* pNewFilename = MemorySim_MapSimulatedAddressToHostAddressForRead(mri4simGetContext()->pMemory,
                                                                                     newFilenameAddress,
                                                                                     newFilenameLength);
        int renameResult = rename(pOrigFilename, pNewFilename);
        SetSemihostReturnValues(renameResult, errno);
    }
    __catch
    {
        SetSemihostReturnValues(-1, EFAULT);
    }
    FlagSemihostCallAsHandled();
    return 1;
}

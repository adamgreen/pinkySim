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
#include <mockReadWrite.h>
#include <mri.h>
#include <platforms.h>
#include <semihost.h>
#include <string.h>
#include <unistd.h>


static int isConsoleOutput(uint32_t fileDescriptor);
static void writeToConsole(PlatformSemihostParameters* pSemihostParameters);
static int isConsoleInput(uint32_t fileDescriptor);
static void readFromConsole(PlatformSemihostParameters* pSemihostParameters);
static int isConsoleFile(uint32_t fileDescriptor);


int handleNewlibSemihostWriteRequest(PlatformSemihostParameters* pSemihostParameters)
{
    TransferParameters parameters;

    if (isConsoleOutput(pSemihostParameters->parameter1))
    {
        writeToConsole(pSemihostParameters);
        if (Platform_CommIsWaitingForGdbToConnect())
        {
            FlagSemihostCallAsHandled();
            return 1;
        }
    }

    parameters.fileDescriptor = pSemihostParameters->parameter1;
    parameters.bufferAddress = pSemihostParameters->parameter2;
    parameters.bufferSize = pSemihostParameters->parameter3;

    return IssueGdbFileWriteRequest(&parameters);
}

static int isConsoleOutput(uint32_t fileDescriptor)
{
    return fileDescriptor == STDOUT_FILENO || fileDescriptor == STDERR_FILENO;
}

static void writeToConsole(PlatformSemihostParameters* pSemihostParameters)
{
    int32_t  file = pSemihostParameters->parameter1;
    uint32_t address = pSemihostParameters->parameter2;
    uint32_t size = pSemihostParameters->parameter3;
    while (size)
    {
        uint8_t byte = Platform_MemRead8((void*)(size_t)address);
        write(file, &byte, sizeof(byte));
        address++;
        size--;
    }
    SetSemihostReturnValues(pSemihostParameters->parameter3, 0);
}

int handleNewlibSemihostReadRequest(PlatformSemihostParameters* pSemihostParameters)
{
    TransferParameters parameters;

    if (isConsoleInput(pSemihostParameters->parameter1)  && Platform_CommIsWaitingForGdbToConnect())
    {
        readFromConsole(pSemihostParameters);
        FlagSemihostCallAsHandled();
        return 1;
    }

    parameters.fileDescriptor = pSemihostParameters->parameter1;
    parameters.bufferAddress = pSemihostParameters->parameter2;
    parameters.bufferSize = pSemihostParameters->parameter3;

    return IssueGdbFileReadRequest(&parameters);
}

static int isConsoleInput(uint32_t fileDescriptor)
{
    return fileDescriptor == STDIN_FILENO;
}

static void readFromConsole(PlatformSemihostParameters* pSemihostParameters)
{
    int32_t  file = pSemihostParameters->parameter1;
    uint32_t address = pSemihostParameters->parameter2;
    uint32_t size = pSemihostParameters->parameter3;
    ssize_t  readResult = -1;
    uint8_t* pBuffer;
    uint8_t* pCurr;

    pBuffer = malloc(size);
    if (!pBuffer)
    {
        SetSemihostReturnValues(-1, ENOMEM);
        return;
    }
    readResult = read(file, pBuffer, size);
    if (readResult == -1)
    {
        free(pBuffer);
        SetSemihostReturnValues(-1, errno);
        return;
    }

    pCurr = pBuffer;
    size = readResult;
    while (size--)
        Platform_MemWrite8((void*)(size_t)address++, *pCurr++);
    SetSemihostReturnValues(readResult, 0);
    free(pBuffer);
}

int handleNewlibSemihostOpenRequest(PlatformSemihostParameters* pSemihostParameters)
{
    OpenParameters parameters;

    parameters.filenameAddress = pSemihostParameters->parameter1;
    parameters.filenameLength = pSemihostParameters->parameter4;
    parameters.flags = pSemihostParameters->parameter2;
    parameters.mode = pSemihostParameters->parameter3;

    return IssueGdbFileOpenRequest(&parameters);
}

int handleNewlibSemihostUnlinkRequest(PlatformSemihostParameters* pSemihostParameters)
{
    RemoveParameters parameters;

    parameters.filenameAddress = pSemihostParameters->parameter1;
    parameters.filenameLength = pSemihostParameters->parameter2;

    return IssueGdbFileUnlinkRequest(&parameters);
}

int handleNewlibSemihostLSeekRequest(PlatformSemihostParameters* pSemihostParameters)
{
    SeekParameters parameters;

    if (isConsoleFile(pSemihostParameters->parameter1))
    {
        SetSemihostReturnValues(-1, EBADF);
        FlagSemihostCallAsHandled();
        return 1;
    }

    parameters.fileDescriptor = pSemihostParameters->parameter1;
    parameters.offset = pSemihostParameters->parameter2;
    parameters.whence = pSemihostParameters->parameter3;

    return IssueGdbFileSeekRequest(&parameters);
}

int handleNewlibSemihostCloseRequest(PlatformSemihostParameters* pSemihostParameters)
{
    if (isConsoleFile(pSemihostParameters->parameter1))
    {
        SetSemihostReturnValues(0, 0);
        FlagSemihostCallAsHandled();
        return 1;
    }

    return IssueGdbFileCloseRequest(pSemihostParameters->parameter1);
}

int handleNewlibSemihostFStatRequest(PlatformSemihostParameters* pSemihostParameters)
{
    if (isConsoleFile(pSemihostParameters->parameter1))
    {
        SetSemihostReturnValues(-1, 0);
        FlagSemihostCallAsHandled();
        return 1;
    }
    return IssueGdbFileFStatRequest(pSemihostParameters->parameter1, pSemihostParameters->parameter2);
}

static int isConsoleFile(uint32_t fileDescriptor)
{
    return isConsoleOutput(fileDescriptor) || isConsoleInput(fileDescriptor);
}

int handleNewlibSemihostStatRequest(PlatformSemihostParameters* pSemihostParameters)
{
    StatParameters parameters;

    parameters.filenameAddress = pSemihostParameters->parameter1;
    parameters.filenameLength = pSemihostParameters->parameter3;
    parameters.fileStatBuffer = pSemihostParameters->parameter2;
    return IssueGdbFileStatRequest(&parameters);
}

int handleNewlibSemihostRenameRequest(PlatformSemihostParameters* pSemihostParameters)
{
    RenameParameters parameters;

    parameters.origFilenameAddress = pSemihostParameters->parameter1;
    parameters.origFilenameLength = pSemihostParameters->parameter3;
    parameters.newFilenameAddress = pSemihostParameters->parameter2;
    parameters.newFilenameLength = pSemihostParameters->parameter4;
    return IssueGdbFileRenameRequest(&parameters);
}

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
#include <FileFailureInject.h>
#include <MemorySim.h>
#include <MallocFailureInject.h>
#include <pinkySimCommandLine.h>
#include <printfSpy.h>
#include <SocketIComm.h>
#include <string.h>
#include <stdio.h>
#include <version.h>


#define READ_ONLY  1
#define READ_WRITE 0


static void displayCopyrightNotice(void)
{
    printf("pinkySim - ARMv6-M Simulator (" VERSION_STRING ")\n\n"
           COPYRIGHT_NOTICE
           "\n");
}

static void displayUsage(void)
{
    printf("Usage: pinkySim [--ram baseAddress size] [--flash baseAddress size] [--gdbPort tcpPortNumber]\n"
           "                [--breakOnStart] imageFilename.bin [args]\n"
           "Where: --ram is used to specify an address range that should be treated as read-write.  More than one of\n"
           "         these can be specified on the command line to create multiple read-write memory regions.\n"
           "       --flash is used to specify and address range that should be treated as read-only.  More than one of\n"
           "         these can be specified on the command line to create multiple read-only memory regions.\n"
           "       --gdbPort can be used to override the default TCP/IP port of 3333 for listening to GDB connections.\n"
           "       --breakOnStart can be used to have the simulator halt at the beginning of the reset handler and\n"
           "         wait for GDB to connect.\n"
           "       imageFilename.bin is the required name of the image to be loaded into memory starting at address\n"
           "         0x00000000.  By default a read-only memory region is created starting at address 0x00000000 and\n"
           "         extends large enough to contain the whole image file.  A read-write section will be created\n"
           "         based on the initial stack pointer found in the first word of the image file.  This section will\n"
           "         start at the nearest 256MB page below this initial SP value and extend to the address just below\n"
           "         this initial SP value. This behaviour can be overridden by specifying --ram and --flash options\n"
           "         on the command line.  Execution will start at the address found in the second word of this image.\n"
           "       [args] are optional arguments to be passed into program running under simulation.\n");
}


static int parseArgument(pinkySimCommandLine* pThis, int index, int argc, const char** ppArgs);
static int hasDoubleDashPrefix(const char* pArgument);
static int parseFlagArgument(pinkySimCommandLine* pThis, int argc, const char** ppArgs);
static int parseFlashOption(pinkySimCommandLine* pThis, int argc, const char** ppArgs);
static int parseRegionOption(pinkySimCommandLine* pThis, int argc, const char** ppArgs, int readOnly);
static int parseRamOption(pinkySimCommandLine* pThis, int argc, const char** ppArgs);
static int parseBreakOnStartOption(pinkySimCommandLine* pThis);
static int parseGdbPortOption(pinkySimCommandLine* pThis, int argc, const char** ppArgs);
static int parseFilenameArgument(pinkySimCommandLine* pThis, int index, int argc, const char* pArgument);
static void throwIfRequiredArgumentNotSpecified(pinkySimCommandLine* pThis);
static void loadImageFile(pinkySimCommandLine* pThis);
static long getFileSize(FILE* pFile);


__throws void pinkySimCommandLine_Init(pinkySimCommandLine* pThis, int argc, const char** argv)
{
    __try
    {
        int index = 0;

        memset(pThis, 0, sizeof(*pThis));
        pThis->pMemory = MemorySim_Init();
        pThis->gdbPort = SOCKET_ICOMM_DEFAULT_PORT;
        while (argc)
        {
            int argumentsUsed = parseArgument(pThis, index, argc, argv);
            argc -= argumentsUsed;
            argv += argumentsUsed;
            index += argumentsUsed;
        }
        throwIfRequiredArgumentNotSpecified(pThis);
        loadImageFile(pThis);
    }
    __catch
    {
        displayCopyrightNotice();
        displayUsage();
        MemorySim_Uninit(pThis->pMemory);
        pThis->pMemory = NULL;
        __rethrow;
    }
}

static int parseArgument(pinkySimCommandLine* pThis, int index, int argc, const char** ppArgs)
{
    if (hasDoubleDashPrefix(*ppArgs))
        return parseFlagArgument(pThis, argc, ppArgs);
    else
        return parseFilenameArgument(pThis, index, argc, *ppArgs);
}

static int hasDoubleDashPrefix(const char* pArgument)
{
    return pArgument[0] == '-' && pArgument[1] == '-';
}

static int parseFlagArgument(pinkySimCommandLine* pThis, int argc, const char** ppArgs)
{
    if (0 == strcasecmp(*ppArgs, "--flash"))
        return parseFlashOption(pThis, argc - 1, &ppArgs[1]);
    else if (0 == strcasecmp(*ppArgs, "--ram"))
        return parseRamOption(pThis, argc - 1, &ppArgs[1]);
    else if (0 == strcasecmp(*ppArgs, "--breakOnStart"))
        return parseBreakOnStartOption(pThis);
    else if (0 == strcasecmp(*ppArgs, "--gdbPort"))
        return parseGdbPortOption(pThis, argc - 1, &ppArgs[1]);
    else
        __throw(invalidArgumentException);
}

static int parseFlashOption(pinkySimCommandLine* pThis, int argc, const char** ppArgs)
{
    return parseRegionOption(pThis, argc, ppArgs, READ_ONLY);
}

static int parseRegionOption(pinkySimCommandLine* pThis, int argc, const char** ppArgs, int readOnly)
{
    uint32_t baseAddress = 0;
    uint32_t size = 0;
    
    if (argc < 2)
        __throw(invalidArgumentException);
    
    baseAddress = strtoul(ppArgs[0], NULL, 0);
    size = strtoul(ppArgs[1], NULL, 0);
    MemorySim_CreateRegion(pThis->pMemory, baseAddress, size);
    if (readOnly)
        MemorySim_MakeRegionReadOnly(pThis->pMemory, baseAddress);
    pThis->manualMemoryRegions = 1;
    return 3;
}

static int parseRamOption(pinkySimCommandLine* pThis, int argc, const char** ppArgs)
{
    return parseRegionOption(pThis, argc, ppArgs, READ_WRITE);
}

static int parseBreakOnStartOption(pinkySimCommandLine* pThis)
{
    pThis->breakOnStart = 1;
    return 1;
}

static int parseGdbPortOption(pinkySimCommandLine* pThis, int argc, const char** ppArgs)
{
    uint32_t portNumber = 0;
    
    if (argc < 1)
        __throw(invalidArgumentException);
    
    portNumber = strtoul(ppArgs[0], NULL, 0);
    if (portNumber > 0xFFFF)
        __throw(invalidArgumentException);
    pThis->gdbPort = portNumber;
    return 2;
}

static int parseFilenameArgument(pinkySimCommandLine* pThis, int index, int argc, const char* pArgument)
{
    pThis->pImageFilename = pArgument;
    pThis->argIndexOfImageFilename = index;
    return argc;
}

static void throwIfRequiredArgumentNotSpecified(pinkySimCommandLine* pThis)
{
    if (!pThis->pImageFilename)
        __throw(invalidArgumentException);
}

static void loadImageFile(pinkySimCommandLine* pThis)
{
    FILE* volatile pFile = NULL;
    char* volatile pBuffer = NULL;
    
    __try
    {
        long   fileSize = 0;
        size_t bytesRead = 0;
    
        pFile = fopen(pThis->pImageFilename, "r");
        if (!pFile)
            __throw(fileException);
        fileSize = getFileSize(pFile);

        pBuffer = malloc(fileSize);
        if (!pBuffer)
            __throw(outOfMemoryException);
    
        bytesRead = fread(pBuffer, 1, fileSize, pFile);
        if ((long)bytesRead != fileSize)
            __throw(fileException);
    
        if (pThis->manualMemoryRegions)
            MemorySim_LoadFromFlashImage(pThis->pMemory, pBuffer, fileSize);
        else
            MemorySim_CreateRegionsFromFlashImage(pThis->pMemory, pBuffer, fileSize);

        free(pBuffer);
        fclose(pFile);
    }
    __catch
    {
        free(pBuffer);
        if (pFile)
            fclose(pFile);
        __rethrow;
    }
}

static long getFileSize(FILE* pFile)
{
    int    result = -1;
    long   fileSize = 0;

    result = fseek(pFile, 0, SEEK_END);
    if (result == -1)
        __throw(fileException);
    
    fileSize = ftell(pFile);
    if (fileSize < 0)
        __throw(fileException);
    
    result = fseek(pFile, 0, SEEK_SET);
    if (result == -1)
        __throw(fileException);
        
    return fileSize;
}


void pinkySimCommandLine_Uninit(pinkySimCommandLine* pThis)
{
    MemorySim_Uninit(pThis->pMemory);
}

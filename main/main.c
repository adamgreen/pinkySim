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
#include <assert.h>
#include <CodeCoverage.h>
#include <mri4sim.h>
#include <pinkySimCommandLine.h>
#include <SocketIComm.h>
#include <stdio.h>
#include <string.h>


static void copyCommandLineArgumentsToStack(PinkySimContext* pContext,
                                           int               argc,
                                           const char**      argv,
                                           int               argIndexOfImageFilename);
static void copyStringToIMemory(IMemory* pMem, uint32_t destAddress, const char* pSrc);
static uint32_t roundDownToNearestDoubleWord(uint32_t value);
static void waitingForGdbToConnect(void);
static void runCodeCoverageIfRequested(pinkySimCommandLine* pCommandLine);


int main(int argc, const char** argv)
{
    int                 returnValue = 0;
    IComm*              pComm = NULL;
    pinkySimCommandLine commandLine;

    __try
    {
        pinkySimCommandLine_Init(&commandLine, argc-1, argv+1);
        pComm = SocketIComm_Init(commandLine.gdbPort, waitingForGdbToConnect);
        mri4simInit(commandLine.pMemory);
        copyCommandLineArgumentsToStack(mri4simGetContext(), argc-1, argv+1, commandLine.argIndexOfImageFilename);
        mri4simRun(pComm, commandLine.breakOnStart);
        returnValue = mri4simGetContext()->R[0];
        runCodeCoverageIfRequested(&commandLine);
    }
    __catch
    {
        if (getExceptionCode() == fileException)
            fprintf(stderr, "Failed to open %s\n", commandLine.pImageFilename);
        returnValue = -1;
    }
    SocketIComm_Uninit(pComm);
    pinkySimCommandLine_Uninit(&commandLine);

    return returnValue;
}

static void copyCommandLineArgumentsToStack(PinkySimContext* pContext,
                                           int               argc,
                                           const char**      argv,
                                           int               argIndexOfImageFilename)
{
    uint32_t ptrsBase;
    uint32_t ptrsCurr;
    uint32_t dataCurr;
    int      i;

    argc -= argIndexOfImageFilename;
    argv += argIndexOfImageFilename;
    ptrsBase = pContext->spMain - argc * sizeof(uint32_t);
    ptrsCurr = ptrsBase;
    dataCurr = ptrsBase;
    __try
    {
        for (i = 0 ; i < argc ; i++)
        {
            size_t len = strlen(argv[i]) + 1;
            dataCurr -= len;
            IMemory_Write32(pContext->pMemory, ptrsCurr, dataCurr);
            copyStringToIMemory(pContext->pMemory, dataCurr, argv[i]);
            ptrsCurr += sizeof(uint32_t);
        }
        assert( ptrsCurr == pContext->spMain );
        pContext->spMain = roundDownToNearestDoubleWord(dataCurr);
        pContext->R[0] = argc;
        pContext->R[1] = ptrsBase;

    }
    __catch
    {
        fprintf(stderr, "Failed to copy command line arguments to top of stack.\n");
        __rethrow;
    }
}

static void copyStringToIMemory(IMemory* pMem, uint32_t destAddress, const char* pSrc)
{
    do
    {
        IMemory_Write8(pMem, destAddress, *pSrc);
        destAddress++;
    } while (*pSrc++);
}

static uint32_t roundDownToNearestDoubleWord(uint32_t value)
{
    return value & ~(8 - 1);
}

static void waitingForGdbToConnect(void)
{
    printf("\nWaiting for GDB to connect...\n");
}

static void runCodeCoverageIfRequested(pinkySimCommandLine* pCommandLine)
{
    if (!pCommandLine->pCoverageElfFilename)
        return;

    __try
    {
        CodeCoverage_Run(pCommandLine->pCoverageElfFilename,
                         pCommandLine->pMemory,
                         pCommandLine->pCoverageResultsDirectory,
                         pCommandLine->ppCoverageRestrictPaths,
                         pCommandLine->coverageRestrictPathCount);
        printf("\nCode coverage results can be found in %s.\n", pCommandLine->pCoverageResultsDirectory);
    }
    __catch
    {
        fprintf(stderr, "\n");
        if (getExceptionCode() == outOfMemoryException)
            fprintf(stderr, "Failed to allocate memory for processing code coverage results.\n");
        else
            fprintf(stderr, "%s\n", CodeCoverage_GetErrorText());
        fprintf(stderr, "Failed to successfully process code coverage results.\n");
        __throw(coverageException);
    }
}

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
#include <mri4sim.h>
#include <SocketIComm.h>
#include <stdio.h>
#include <pinkySimCommandLine.h>


static void waitingForGdbToConnect(void);


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
        mri4simRun(pComm, commandLine.breakOnStart);
    }
    __catch
    {
        if (getExceptionCode() == fileException)
            fprintf(stderr, "Failed to open %s\n", commandLine.pImageFilename);
        returnValue = 1;
    }
    
    SocketIComm_Uninit(pComm);
    pinkySimCommandLine_Uninit(&commandLine);
    
    return returnValue;
}

static void waitingForGdbToConnect(void)
{
    printf("\nWaiting for GDB to connect...\n");
}
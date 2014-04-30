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
/* Semihost functionality for redirecting stdin/stdout/stderr I/O to the GNU console. */
#include <string.h>
#include <mri.h>
#include <semihost.h>
#include <cmd_file.h>


int handleNewlibSemihostWriteRequest(PlatformSemihostParameters* pSemihostParameters)
{
    TransferParameters parameters;

    parameters.fileDescriptor = pSemihostParameters->parameter1;
    parameters.bufferAddress = pSemihostParameters->parameter2;
    parameters.bufferSize = pSemihostParameters->parameter3;
    
    return IssueGdbFileWriteRequest(&parameters);
}

int handleNewlibSemihostReadRequest(PlatformSemihostParameters* pSemihostParameters)
{
    TransferParameters parameters;

    parameters.fileDescriptor = pSemihostParameters->parameter1;
    parameters.bufferAddress = pSemihostParameters->parameter2;
    parameters.bufferSize = pSemihostParameters->parameter3;
    
    return IssueGdbFileReadRequest(&parameters);
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

    parameters.fileDescriptor = pSemihostParameters->parameter1;    
    parameters.offset = pSemihostParameters->parameter2;
    parameters.whence = pSemihostParameters->parameter3;
    
    return IssueGdbFileSeekRequest(&parameters);
}

int handleNewlibSemihostCloseRequest(PlatformSemihostParameters* pSemihostParameters)
{
    return IssueGdbFileCloseRequest(pSemihostParameters->parameter1);
}

int handleNewlibSemihostFStatRequest(PlatformSemihostParameters* pSemihostParameters)
{
    return IssueGdbFileFStatRequest(pSemihostParameters->parameter1, pSemihostParameters->parameter2);
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

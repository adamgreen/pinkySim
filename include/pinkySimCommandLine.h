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
#ifndef _PINKSIM_COMMANDLINE_H_
#define _PINKSIM_COMMANDLINE_H_

#include <try_catch.h>
#include <IMemory.h>


typedef struct pinkySimCommandLine
{
    const char*  pImageFilename;
    const char*  pCoverageElfFilename;
    const char*  pCoverageResultsDirectory;
    const char** ppCoverageRestrictPaths;
    IMemory*     pMemory;
    int          breakOnStart;
    int          manualMemoryRegions;
    int          argIndexOfImageFilename;
    uint32_t     coverageRestrictPathCount;
    uint16_t     gdbPort;
} pinkySimCommandLine;


__throws void pinkySimCommandLine_Init(pinkySimCommandLine* pThis, int argc, const char** argv);
         void pinkySimCommandLine_Uninit(pinkySimCommandLine* pThis);


#endif /* _PINKSIM_COMMANDLINE_H_ */

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
#ifndef _CODE_COVERAGE_H_
#define _CODE_COVERAGE_H_

#include <MemorySim.h>
#include <try_catch.h>

__throws void CodeCoverage_Run(const char* pElfFilename,
                               IMemory* pMemory,
                               const char* pOutputDir,
                               const char** ppRestrictPaths,
                               int restrictPathCount);
const char* CodeCoverage_GetErrorText(void);

#endif /* _CODE_COVERAGE_H_ */

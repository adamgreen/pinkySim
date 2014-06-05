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
#ifndef _ELF_LINES_H_
#define _ELF_LINES_H_

#include <stdint.h>
#include <try_catch.h>

/* Grow the ElfLines::pLines array by this number of lines at a time. */
#define ELFLINE_GROW_ALLOC    (16 * 1024)

typedef struct ElfLine
{
    const char* pFilename;
    uint32_t    lineNumber;
    uint32_t    address;
} ElfLine;

typedef struct ElfFilename
{
    struct ElfFilename* pNext;
    const char*         pFilename;
    size_t              filenameLength;
    char                fullFilename[1];
} ElfFilename;

typedef struct ElfLines
{
    ElfLine*     pLines;
    ElfFilename* pFilenameHead;
    uint32_t     lineCount;
    uint32_t     allocatedLines;
} ElfLines;

__throws ElfLines* ElfLines_Parse(const char* pElfFilename);
         void      ElfLines_Uninit(ElfLines* pLines);

#endif /* _ELF_LINES_H_ */

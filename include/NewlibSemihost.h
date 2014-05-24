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
#ifndef _NEWLIB_SEMIHOST_H_
#define _NEWLIB_SEMIHOST_H_

/* Newlib APIs are semihosted to GDB by issuing appropriate BKPT instructions.  The following list gives the immediate
   constant to be used with BKPT for each operation. */
#define NEWLIB_EXIT     0xFF
#define NEWLIB_WRITE    0xFE
#define NEWLIB_READ     0xFD
#define NEWLIB_OPEN     0xFC
#define NEWLIB_RENAME   0xFB
#define NEWLIB_UNLINK   0xFA
#define NEWLIB_STAT     0xF9
#define NEWLIB_LSEEK    0xF8
#define NEWLIB_CLOSE    0xF7
#define NEWLIB_FSTAT    0xF6

#define NEWLIB_MIN      0xF6
#define NEWLIB_MAX      0xFF

/* Definitions only required from C code. */
#if !__ASSEMBLER__

#include <stdint.h>

/* The more common struct stat fields that will be transferred from host to target. */
typedef struct CommonStat
{
    uint32_t mode;
    uint32_t size;
    uint32_t blksize;
    uint32_t blocks;
} CommonStat;

#endif /* !__ASSEMBLER__ */
#endif /* _NEWLIB_SEMIHOST_H_ */

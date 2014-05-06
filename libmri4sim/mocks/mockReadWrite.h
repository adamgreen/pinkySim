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
/* Module for spying on printf output from code under test. */
#ifndef _MOCK_READ_WRITE_H
#define _MOCK_READ_WRITE_H

#include <unistd.h>


/* Pointer to I/O routines which can intercepted by this module. */
extern ssize_t (*hook_read)(int fildes, void *buf, size_t nbyte);
extern ssize_t (*hook_write)(int fildes, const void *buf, size_t nbyte);


#undef  read
#define read  hook_read
#undef  write
#define write hook_write


void        mockReadWrite_SetReadData(const void* pvData, size_t dataSize);
void        mockReadWrite_SetReadToFail(int result, int err);
void        mockReadWrite_CreateWriteBuffer(size_t bufferSize);
const char* mockReadWrite_GetStdOutData(void);
const char* mockReadWrite_GetStdErrData(void);
void        mockReadWrite_Uninit(void);


#endif /* _MOCK_READ_WRITE_H */

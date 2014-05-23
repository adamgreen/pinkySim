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
#ifndef _MOCK_FILE_IO_H
#define _MOCK_FILE_IO_H

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>


/* Pointer to I/O routines which can intercepted by this module. */
extern int     (*hook_open)(const char *path, int oflag, ...);
extern ssize_t (*hook_read)(int fildes, void *buf, size_t nbyte);
extern ssize_t (*hook_write)(int fildes, const void *buf, size_t nbyte);
extern off_t   (*hook_lseek)(int fildes, off_t offset, int whence);
extern int     (*hook_close)(int fildes);
extern int     (*hook_unlink)(const char *path);
extern int     (*hook_rename)(const char *oldPath, const char *newPath);
extern int     (*hook_fstat)(int fildes, struct stat *buf);
extern int     (*hook_stat)(const char* path, struct stat* buf);


#undef  open
#define open  hook_open
#undef  read
#define read  hook_read
#undef  write
#define write hook_write
#undef  lseek
#define lseek hook_lseek
#undef  close
#define close hook_close
#undef  unlink
#define unlink hook_unlink
#undef  rename
#define rename hook_rename
#undef  fstat
#define fstat hook_fstat


void        mockFileIo_SetOpenToFail(int result, int err);
void        mockFileIo_SetReadData(const void* pvData, size_t dataSize);
void        mockFileIo_SetReadToFail(int result, int err);
void        mockFileIo_CreateWriteBuffer(size_t bufferSize);
const char* mockFileIo_GetStdOutData(void);
const char* mockFileIo_GetStdErrData(void);
void        mockFileIo_SetLSeekToFail(int result, int err);
void        mockFileIo_SetCloseToFail(int result, int err);
void        mockFileIo_SetUnlinkToFail(int result, int err);
void        mockFileIo_SetRenameToFail(int result, int err);
void        mockFileIo_SetFStatCallResults(int result, int err, struct stat* pStat);
void        mockFileIo_SetStatCallResults(int result, int err, struct stat* pStat);
void        mockFileIo_Uninit(void);


#endif /* _MOCK_FILE_IO_H */

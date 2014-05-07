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
#include <errno.h>
#include <stdint.h>
#include "SemihostThunks.h"


/* Turn off the errno macro and use actual external global variable instead. */
#undef errno
extern int errno;


/* Linker defined symbol to be used by _sbrk for where dynamically heap should start. */
extern uint32_t __end__[1];


/* File system related syscalls. */
int _open(const char *pFilename, int flags, int mode)
{
    return semihostOpen(pFilename, flags, mode);
}

int rename(const char *pOldFilename, const char *pNewFilename)
{
    return semihostRename(pOldFilename, pNewFilename);
}

int _unlink(const char *pFilename)
{
    return semihostUnlink(pFilename);
}

int _stat(const char *pFilename, struct stat *pStat)
{
    return semihostStat(pFilename, pStat);
}


/* File handle related syscalls. */
int _read(int file, char *ptr, int len)
{
    return semihostRead(file, ptr, len);
}

int _write(int file, char *ptr, int len)
{
    return semihostWrite(file, ptr, len);
}

int _isatty(int file)
{
    if (file >= 0 && file <= 2)
        return 1;
    return 0;
}

int _lseek(int file, int ptr, int dir)
{
    return semihostLSeek(file, ptr, dir);
}

int _close(int file)
{
    return semihostClose(file);
}

int _fstat(int file, struct stat *st)
{
    return semihostFStat(file, st);
}


/* Dynamic memory allocation related syscalls. */
__attribute__( ( always_inline ) ) static inline uint32_t __get_MSP(void)
{
  register uint32_t result;

  __asm volatile ("MRS %0, msp\n" : "=r" (result) );
  return(result);
}

caddr_t _sbrk(int incr)
{
    static unsigned char* heap = (unsigned char*)__end__;
    unsigned char*        prevHeap = heap;
    unsigned char*        newHeap = heap + incr;

    if (newHeap >= (unsigned char*)__get_MSP())
    {
        errno = ENOMEM;
        return (caddr_t)-1;
    }

    heap = newHeap;
    return (caddr_t)prevHeap;
}


/* Other syscalls. */
void _exit(int code)
{
    semihostExit(code);
    for (;;)
    {
    }
}

int _getpid()
{
    return 1;
}

int _kill(int pid, int sig)
{
    errno = EINVAL;
    return -1;
}

int _wait(int *status)
{
    errno = ECHILD;
    return -1;
}

int _system(const char* pCommand)
{
    if (NULL == pCommand)
    {
        /* There is no command interpreter on the embedded device. */
        return 0;
    }
    else
    {
        /* Indicate that command couldn't be executed since there is no command interpreter. */
        errno = ENOENT;
        return -1;
    }
}

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
/* Implementation of semihost assembly language routines to be used to redirect I/O from the newlib library to the
   MRI debug monitor. */
    #include <NewlibSemihost.h>
    
    .text
    .code 16
    .syntax unified

    .global semihostWrite
    .type semihostWrite, function
    /* extern "C" int semihostWrite(int file, char *ptr, int len);
       Issues semihost write call to PC via GDB.
    */
semihostWrite:
    bkpt    #NEWLIB_WRITE
    b       handleError

    .type handleError, function
    /*  Set the global errno if r0 indicates an error was encountered. */
handleError:
    cmp     r0, #0
    bge     1$
    ldr     r2, =errno
    str     r1, [r2, #0]
1$: bx      lr


    .global semihostRead
    .type semihostRead, function
    /* extern "C" int semihostRead(int file, char *ptr, int len);
       Issues semihost read call to PC via GDB.
    */
semihostRead:
    bkpt    #NEWLIB_READ
    b       handleError


    .global semihostOpen
    .type semihostOpen, function
    /* extern "C" int semihostOpen(const char *pFilename, int flags, int mode);
       Issues semihost open file call to PC via GDB.
    */
semihostOpen:
    push    {r4, r5, lr}
    mov     r4, r0
    bl      strlen_of_r4_result_in_r3
    bkpt    #NEWLIB_OPEN
    bl      handleError
    pop     {r4, r5, pc}

    .type strlen_of_r4_result_in_r3, function
    /* Takes a pointer to a string in r4 and returns the length of the string (including NULL terminator) in r3. */
    /* NOTE: Caller should push r4 and r5 before calling this asm routine.  Not callable from C/C++ code. */
strlen_of_r4_result_in_r3:
    movs    r3, #0
1$: ldrb    r5, [r4, r3]
    adds    r3, r3, #1
    cmp     r5, #0
    bne     1$
    bx      lr
    

    .global semihostRename
    .type semihostRename, function
    /* extern "C" int semihostRename(const char *pOldFilename, const char *pNewFilename);
       Issues file rename call to PC via GDB.
    */
semihostRename:
    push    {r4, r5, lr}
    mov     r4, r0
    bl      strlen_of_r4_result_in_r3
    mov     r2, r3
    mov     r4, r1
    bl      strlen_of_r4_result_in_r3
    bkpt    #NEWLIB_RENAME
    bl      handleError
    pop     {r4, r5, pc}
    
    .global semihostUnlink
    .type semihostUnlink, function
    /* extern "C" int semihostUnlink(const char *pFilename);
       Issues file delete (unlink) call to PC via GDB.
    */
semihostUnlink:
    push    {r4, r5, lr}
    mov     r4, r0
    bl      strlen_of_r4_result_in_r3
    mov     r1, r3
    bkpt    #NEWLIB_UNLINK
    bl      handleError
    pop     {r4, r5, pc}
    
    
    .global semihostStat
    .type semihostStat, function
    /* extern "C" int semihostStat(const char *pFilename, struct stat *pStat);
       Issues stat call to PC via GDB.
    */
semihostStat:
    push    {r4, r5, lr}
    mov     r4, r0
    bl      strlen_of_r4_result_in_r3
    mov     r2, r3
    bkpt    #NEWLIB_STAT
    bl      handleError
    pop     {r4, r5, pc}
    
    
    .global semihostLSeek
    .type semihostLSeek, function
    /* extern "C" int semihostLSeek(int file, int offset, int whence);
       Issues seek call to PC via GDB.
    */
semihostLSeek:
    bkpt    #NEWLIB_LSEEK
    b       handleError
    
    
    .global semihostClose
    .type semihostClose, function
    /* extern "C" int semihostClose(int file);
       Issues file close call to PC via GDB.
    */
semihostClose:
    bkpt    #NEWLIB_CLOSE
    b       handleError
    
    
    .global semihostFStat
    .type semihostFStat, function
    /* extern "C" int semihostFStat(int file, struct stat *pStat);
       Issues stat call to PC via GDB. 
    */
semihostFStat:
    bkpt    #NEWLIB_FSTAT
    b       handleError
    
    
    .global semihostExit
    .type semihostExit, function
    /* extern "C" void semihostExit(int code);
       Sends an exit request to pinkySim. 
    */
semihostExit:
    bkpt    #NEWLIB_EXIT
    bx      lr

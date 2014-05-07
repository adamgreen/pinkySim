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
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>


char*   g_pLastOutput = NULL;
char*   g_pPreviousOutput = NULL;
char*   g_pLastErrorOutput = NULL;
size_t  g_bufferSize = 0;
size_t  g_callCount = 0;
FILE*   g_pFile = NULL;
int (*hook_printf)(const char* pFormat, ...) = printf;
int (*hook_fprintf)(FILE* pFile, const char* pFormat, ...) = fprintf;

void printfSpy_Unhook(void);

static void _AllocateAndInitBuffers(size_t BufferSize)
{
    g_bufferSize = BufferSize + 1;
    g_pLastOutput = malloc(g_bufferSize);
    assert(NULL != g_pLastOutput);
    g_pPreviousOutput = malloc(g_bufferSize);
    assert(NULL != g_pPreviousOutput);
    g_pLastErrorOutput = malloc(g_bufferSize);
    assert(NULL != g_pLastErrorOutput);
    g_pLastOutput[0] = '\0';
    g_pPreviousOutput[0] = '\0';
    g_pLastErrorOutput[0] = '\0';
    g_pFile = NULL;
}

static void _FreeBuffer(void)
{
    free(g_pLastErrorOutput);
    free(g_pPreviousOutput);
    free(g_pLastOutput);
    g_pLastOutput = NULL;
    g_pPreviousOutput = NULL;
    g_pLastErrorOutput = NULL;
    g_bufferSize = 0;
    g_pFile = NULL;
}

static int mock_common(FILE* pFile, const char* pFormat, va_list valist)
{
    int     WrittenSize = -1;

    strcpy(g_pPreviousOutput, g_pLastOutput);
    WrittenSize = vsnprintf(g_pLastOutput,
                            g_bufferSize,
                            pFormat,
                            valist);
    if (pFile == stderr)
        strcpy(g_pLastErrorOutput, g_pLastOutput);
    g_callCount++;
    g_pFile = pFile;
    return WrittenSize;
}

static int mock_printf(const char* pFormat, ...)
{
    va_list valist;
    va_start(valist, pFormat);
    return mock_common(stdout, pFormat, valist);
}

static int mock_fprintf(FILE* pFile, const char* pFormat, ...)
{
    va_list valist;
    va_start(valist, pFormat);
    return mock_common(pFile, pFormat, valist);
}

static void setHookFunctionPointers(void)
{
    hook_printf = mock_printf;
    hook_fprintf = mock_fprintf;
}

static void restoreHookFunctionPointers(void)
{
    hook_printf = printf;
    hook_fprintf = fprintf;
}


/********************/
/* Public routines. */
/********************/
void printfSpy_Hook(size_t BufferSize)
{
    printfSpy_Unhook();

    _AllocateAndInitBuffers(BufferSize);
    g_callCount = 0;
    setHookFunctionPointers();
}

void printfSpy_Unhook(void)
{
    restoreHookFunctionPointers();
    _FreeBuffer();
}

const char* printfSpy_GetLastOutput(void)
{
    return g_pLastOutput;
}

const char* printfSpy_GetPreviousOutput(void)
{
    return g_pPreviousOutput;
}

const char* printfSpy_GetLastErrorOutput(void)
{
    return g_pLastErrorOutput;
}

FILE* printfSpy_GetLastFile(void)
{
    return g_pFile;
}

size_t printfSpy_GetCallCount(void)
{
    return g_callCount;
}

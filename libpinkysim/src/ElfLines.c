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
#include <common.h>
#include <ElfLines.h>
#include <MallocFailureInject.h>
#include <mockFileIo.h>
#include <stdio.h>
#include <string.h>


typedef struct ParseState
{
    int didLastLineContainAddress;
    int isDiscardedFunction;
    int wasDiscarding;
} ParseState;

static FILE* openPipeToReadElf(const char* pElfFilename);
static void* allocateAndThrowOnOutOfMemory(size_t size);
static void* allocateZeroAndThrowOnOutOfMemory(size_t size);
static void parseLines(ElfLines* pLines, FILE* pFile);
static int parseLine(ElfLines* pLines, FILE* pFile, ParseState* pState);
static int didLastLineHaveAddressOfZero(ElfLines* pLines, ParseState* pState);
static int didLastLineHaveAddressThatShouldNotBeDiscarded(ParseState* pState);
static int doesLineContainFullFilename(const char* pLineText);
static void addFullFilenameToHeadOfLinkedList(ElfLines* pLines, const char* pLineText);
static int doesLineContainLineInfo(ElfLines* pLines, const char* pLineText);
static void addLineInfo(ElfLines* pLines, ParseState* pState, char* pLineText);
static void growLinesArrayIfNeeded(ElfLines* pLines);
static int compareLines(const void* pv1, const void* pv2);


__throws ElfLines* ElfLines_Parse(const char* pElfFilename)
{
    FILE* volatile        pFile = NULL;
    ElfLines* volatile    pLines = NULL;

    __try
    {
        pFile = openPipeToReadElf(pElfFilename);
        pLines = allocateZeroAndThrowOnOutOfMemory(sizeof(*pLines));
        parseLines(pLines, pFile);
    }
    __catch
    {
        ElfLines_Uninit(pLines);
        pLines = NULL;
    }
    if (pFile)
        pclose(pFile);
    if (pLines)
        qsort(pLines->pLines, pLines->lineCount, sizeof(*pLines->pLines), compareLines);
    return pLines;
}

static FILE* openPipeToReadElf(const char* pElfFilename)
{
    static const char     commandPrefix[] = "arm-none-eabi-readelf -wL ";
    size_t                commandLineLength = sizeof(commandPrefix) + strlen(pElfFilename);
    char* volatile        pCommandLine = NULL;
    FILE* volatile        pFile = NULL;

    __try
    {
        pCommandLine = allocateAndThrowOnOutOfMemory(commandLineLength);
        snprintf(pCommandLine, commandLineLength, "%s%s", commandPrefix, pElfFilename);

        pFile = popen(pCommandLine, "r");
        if (!pFile)
            __throw(fileException);

        free(pCommandLine);
    }
    __catch
    {
        free(pCommandLine);
        __rethrow;
    }

    return pFile;
}

static void* allocateAndThrowOnOutOfMemory(size_t size)
{
    void* pAlloc = malloc(size);
    if (!pAlloc)
        __throw(outOfMemoryException);
    return pAlloc;
}

static void* allocateZeroAndThrowOnOutOfMemory(size_t size)
{
    void* pAlloc = allocateAndThrowOnOutOfMemory(size);
    memset(pAlloc, 0, size);
    return pAlloc;
}

static void parseLines(ElfLines* pLines, FILE* pFile)
{
    ParseState        state = { FALSE, FALSE, FALSE };

    while (parseLine(pLines, pFile, &state))
    {
    }
    if (!feof(pFile))
        __throw(fileException);
    if (state.wasDiscarding)
        pLines->lineCount--;
}

static int parseLine(ElfLines* pLines, FILE* pFile, ParseState* pState)
{
    char*        pResult = NULL;
    char         buffer[1024];

    pResult = fgets(buffer, sizeof(buffer), pFile);
    if (!pResult)
        strcpy(buffer, "\n");

    if (buffer[0] == '\n')
    {
        pState->wasDiscarding = pState->isDiscardedFunction;
        pState->isDiscardedFunction = didLastLineHaveAddressOfZero(pLines, pState);
    }

    /* Only advance to next line in pLines if the previously update line item isn't to be discarded.
       If we don't advance then there is no line to possibly roll back in caller so set wasDiscarding to FALSE. */
    if (didLastLineHaveAddressThatShouldNotBeDiscarded(pState))
        pLines->lineCount++;
    else
        pState->wasDiscarding = FALSE;
    pState->didLastLineContainAddress = FALSE;

    if (doesLineContainFullFilename(buffer))
        addFullFilenameToHeadOfLinkedList(pLines, buffer);
    else if (doesLineContainLineInfo(pLines, buffer))
        addLineInfo(pLines, pState, buffer);

    return pResult != NULL;
}

static int didLastLineHaveAddressOfZero(ElfLines* pLines, ParseState* pState)
{
    return pState->didLastLineContainAddress && pLines->pLines[pLines->lineCount].address == 0;
}

static int didLastLineHaveAddressThatShouldNotBeDiscarded(ParseState* pState)
{
    return !pState->isDiscardedFunction && pState->didLastLineContainAddress;
}

static const char g_cuLabel[] = "CU: ";

static int doesLineContainFullFilename(const char* pLineText)
{
    return (0 == strncmp(pLineText, g_cuLabel, sizeof(g_cuLabel) - 1));
}

static void addFullFilenameToHeadOfLinkedList(ElfLines* pLines, const char* pLineText)
{
    char*        pShortFilename = NULL;
    ElfFilename* pFilename = NULL;

    /* Filename will exclude leading "CU:" label and trailing ":\n" text. ElfFilename structure already
       includes space for NULL terminator so don't need to account for it here too. */
    size_t filenameLength = strlen(pLineText) - (sizeof(g_cuLabel) - 1 + 2);
    pFilename = allocateAndThrowOnOutOfMemory(sizeof(*pFilename) + filenameLength);
    strncpy(pFilename->fullFilename, pLineText + sizeof(g_cuLabel) - 1, filenameLength);
    pFilename->fullFilename[filenameLength] = '\0';
    pShortFilename = strrchr(pFilename->fullFilename, '/');
    if (!pShortFilename)
        pFilename->pFilename = pFilename->fullFilename;
    else
        pFilename->pFilename = pShortFilename + 1;
    pFilename->filenameLength = strlen(pFilename->pFilename);
    pFilename->pNext = pLines->pFilenameHead;
    pLines->pFilenameHead = pFilename;
}

static int doesLineContainLineInfo(ElfLines* pLines, const char* pLineText)
{
    ElfFilename* pFilename = pLines->pFilenameHead;
    return pFilename && 0 == strncmp(pLineText, pFilename->pFilename, pFilename->filenameLength);
}

static void addLineInfo(ElfLines* pLines, ParseState* pState, char* pLineText)
{
    ElfFilename* pFilename = pLines->pFilenameHead;
    char*        pCurr = pLineText + pFilename->filenameLength;

    growLinesArrayIfNeeded(pLines);
    pLines->pLines[pLines->lineCount].lineNumber = strtoul(pCurr, &pCurr, 0);
    pLines->pLines[pLines->lineCount].address = strtoul(pCurr, &pCurr, 0);
    pLines->pLines[pLines->lineCount].pFilename = pFilename->fullFilename;
    pState->didLastLineContainAddress = TRUE;
    pState->wasDiscarding = FALSE;
}

static void growLinesArrayIfNeeded(ElfLines* pLines)
{
    uint32_t newAllocationCount = pLines->allocatedLines + ELFLINE_GROW_ALLOC;
    ElfLine* pRealloc = NULL;

    if (pLines->lineCount < pLines->allocatedLines)
        return;
    pRealloc = realloc(pLines->pLines, sizeof(*pRealloc) * newAllocationCount);
    if (!pRealloc)
        __throw(outOfMemoryException);
    pLines->pLines = pRealloc;
    pLines->allocatedLines = newAllocationCount;
}

static int compareLines(const void* pv1, const void* pv2)
{
    const ElfLine* p1 = (const ElfLine*)pv1;
    const ElfLine* p2 = (const ElfLine*)pv2;
    int   result = strcmp(p1->pFilename, p2->pFilename);
    if (result == 0)
    {
        if (p1->lineNumber < p2->lineNumber)
            result = -1;
        else if (p1->lineNumber > p2->lineNumber)
            result = 1;
    }
    return result;
}


void ElfLines_Uninit(ElfLines* pLines)
{
    ElfFilename* pCurr;

    if (!pLines)
        return;

    pCurr = pLines->pFilenameHead;
    while (pCurr)
    {
        ElfFilename* pNext = pCurr->pNext;
        free(pCurr);
        pCurr = pNext;
    }
    free(pLines->pLines);
    free(pLines);
}

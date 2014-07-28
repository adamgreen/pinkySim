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
#include <assert.h>
#include <CodeCoverage.h>
#include <common.h>
#include <ElfLines.h>
#include <FileFailureInject.h>
#include <limits.h>
#include <MallocFailureInject.h>
#include <string.h>

typedef struct PrivateData
{
    IMemory*     pMemory;
    ElfLines*    pLines;
    const char*  pOutputDir;
    const char** ppRestrictPaths;
    FILE*        pSourceFile;
    FILE*        pDestFile;
    FILE*        pSummaryFile;
    char*        pOutputFilename;
    const char*  pSourceFilename;
    char*        pSourceFileText;
    char*        pCurr;
    size_t       outputFilenameSize;
    size_t       sourceFileTextSize;
    float        percentCovered;
    int          restrictPathCount;
    uint32_t     currentElfLine;
    uint32_t     currentSourceLine;
    uint32_t     minCount;
} PrivateData;

static char g_errorText[256];

static ElfLines* parseElfAndDisplayMsgOnErrors(const char* pElfFilename);
static void initPrivateData(PrivateData* pData,
                            IMemory* pMemory,
                            const char* pOutputDir,
                            const char** ppRestrictPaths,
                            int restrictPathCount);
static void growOutputFilenameBufferIfNecessary(PrivateData* pData, size_t requiredSize);
static void growBufferIfNecessary(char** ppBuffer, size_t* pBufferSize, size_t requiredSize);
static FILE* openFileAndThrowOnFailure(const char* pFilename, const char* pMode);
static void walkElfLinesToDetermineCodeCoverage(PrivateData* pData);
static void setOutputFilename(PrivateData* pData, const char* pFilename, const char* pExtension);
static void walkSourceFileLines(PrivateData* pData);
static int shouldSkipThisSourceFile(PrivateData* pData);
static void skipLinesForThisSourceFile(PrivateData* pData);
static void openCurrentSourceFileAndReadIntoBuffer(PrivateData* pData);
static void growSourceFileTextBufferIfNecessary(PrivateData* pData, size_t requiredSize);
static void iterateOverLinesInSourceFile(PrivateData* pData);
static const char* getNextSourceLine(PrivateData* pData);
static int isTwoCharacterLineTerminator(char previous, char current);
static int doesCurrentSourceLineMatchCurrentElfLine(PrivateData* pData);
static void iterateOverElfLinesWhichMatchCurrentSourceLine(PrivateData* pData);
static void closeSourceAndDestSourceFiles(PrivateData* pData);
static void uninitPrivateData(PrivateData* pData);


__throws void CodeCoverage_Run(const char* pElfFilename,
                               IMemory* pMemory,
                               const char* pOutputDir,
                               const char** ppRestrictPaths,
                               int restrictPathCount)
{
    PrivateData data;

    __try
    {
        g_errorText[0] = '\0';
        initPrivateData(&data, pMemory, pOutputDir, ppRestrictPaths, restrictPathCount);
        data.pLines = parseElfAndDisplayMsgOnErrors(pElfFilename);
        walkElfLinesToDetermineCodeCoverage(&data);
        uninitPrivateData(&data);
    }
    __catch
    {
        uninitPrivateData(&data);
        __rethrow;
    }

    return;
}

static ElfLines* parseElfAndDisplayMsgOnErrors(const char* pElfFilename)
{
    ElfLines* pLines = NULL;

    __try
    {
        pLines = ElfLines_Parse(pElfFilename);
    }
    __catch
    {
        snprintf(g_errorText, sizeof(g_errorText), "error: Failed to parse line information for %s.", pElfFilename);
        __rethrow;
    }

    return pLines;
}

static void initPrivateData(PrivateData* pData,
                            IMemory* pMemory,
                            const char* pOutputDir,
                            const char** ppRestrictPaths,
                            int restrictPathCount)
{
    memset(pData, 0, sizeof(*pData));
    growOutputFilenameBufferIfNecessary(pData, strlen(pOutputDir) + 256);
    pData->pMemory = pMemory;
    pData->pOutputDir = pOutputDir;
    pData->ppRestrictPaths = ppRestrictPaths;
    pData->restrictPathCount = restrictPathCount;
}

static void growOutputFilenameBufferIfNecessary(PrivateData* pData, size_t requiredSize)
{
    growBufferIfNecessary(&pData->pOutputFilename, &pData->outputFilenameSize, requiredSize);
}

static void growBufferIfNecessary(char** ppBuffer, size_t* pBufferSize, size_t requiredSize)
{
    char* pRealloc = NULL;
    if (requiredSize <= *pBufferSize)
        return;
    pRealloc = realloc(*ppBuffer, requiredSize);
    if (!pRealloc)
        __throw(outOfMemoryException);
    *ppBuffer = pRealloc;
    *pBufferSize = requiredSize;
}

static void walkElfLinesToDetermineCodeCoverage(PrivateData* pData)
{
    setOutputFilename(pData, "summary.txt", NULL);
    pData->pSummaryFile = openFileAndThrowOnFailure(pData->pOutputFilename, "w");
    while (pData->currentElfLine < pData->pLines->lineCount)
        walkSourceFileLines(pData);
}

static void setOutputFilename(PrivateData* pData, const char* pFilename, const char* pExtension)
{
    const char* pBaseFilename = NULL;
    size_t      totalLength = 0;

    if ((pBaseFilename = strrchr(pFilename, '/')) != NULL)
        pFilename = pBaseFilename + 1;
    totalLength = strlen(pData->pOutputDir) + 1 + strlen(pFilename) + 1;
    if (pExtension)
        totalLength += strlen(pExtension) + 1;
    growOutputFilenameBufferIfNecessary(pData, totalLength);
    snprintf(pData->pOutputFilename, totalLength, "%s/%s%s",
             pData->pOutputDir, pFilename, pExtension ? pExtension : "");
}

static FILE* openFileAndThrowOnFailure(const char* pFilename, const char* pMode)
{
    FILE* pFile = fopen(pFilename, pMode);
    if (!pFile)
    {
        snprintf(g_errorText, sizeof(g_errorText), "error: Failed to open %s.", pFilename);
        __throw(fileException);
    }
    return pFile;
}

static void walkSourceFileLines(PrivateData* pData)
{
    pData->currentSourceLine = 1;
    pData->pSourceFilename = pData->pLines->pLines[pData->currentElfLine].pFilename;
    if (shouldSkipThisSourceFile(pData))
    {
        skipLinesForThisSourceFile(pData);
        return;
    }
    openCurrentSourceFileAndReadIntoBuffer(pData);
    setOutputFilename(pData, pData->pSourceFilename, ".cov");
    pData->pDestFile = openFileAndThrowOnFailure(pData->pOutputFilename, "w");

    iterateOverLinesInSourceFile(pData);
    fprintf(pData->pSummaryFile, "%6.2f%%  %s\n", pData->percentCovered, pData->pSourceFilename);

    closeSourceAndDestSourceFiles(pData);
}

static int shouldSkipThisSourceFile(PrivateData* pData)
{
    int i;

    if (!pData->ppRestrictPaths || pData->restrictPathCount == 0)
        return FALSE;
    for (i = 0 ; i < pData->restrictPathCount ; i++)
    {
        if (pData->pSourceFilename == strstr(pData->pSourceFilename, pData->ppRestrictPaths[i]))
            return FALSE;
    }
    return TRUE;
}

static void skipLinesForThisSourceFile(PrivateData* pData)
{
    while (pData->currentElfLine < pData->pLines->lineCount &&
           pData->pLines->pLines[pData->currentElfLine].pFilename == pData->pSourceFilename)
    {
        pData->currentElfLine++;
    }
}

static void openCurrentSourceFileAndReadIntoBuffer(PrivateData* pData)
{
    long        fileSize = 0;
    size_t      bytesRead = 0;

    pData->pSourceFile = openFileAndThrowOnFailure(pData->pSourceFilename, "r");
    fileSize = GetFileSize(pData->pSourceFile);
    growSourceFileTextBufferIfNecessary(pData, fileSize + 1);
    bytesRead = fread(pData->pSourceFileText, 1, fileSize, pData->pSourceFile);
    if ((long)bytesRead != fileSize)
    {
        snprintf(g_errorText, sizeof(g_errorText), "error: Failed to read %s.", pData->pSourceFilename);
        __throw(fileException);
    }
    pData->pSourceFileText[fileSize] = '\0';
    pData->pCurr = pData->pSourceFileText;
}

static void growSourceFileTextBufferIfNecessary(PrivateData* pData, size_t requiredSize)
{
    growBufferIfNecessary(&pData->pSourceFileText, &pData->sourceFileTextSize, requiredSize);
}

static void iterateOverLinesInSourceFile(PrivateData* pData)
{
    uint32_t    executableLineCount = 0;
    uint32_t    executedLineCount = 0;
    const char* pSourceLine = NULL;

    while ((pSourceLine = getNextSourceLine(pData)) != NULL)
    {
        if (doesCurrentSourceLineMatchCurrentElfLine(pData))
        {
            iterateOverElfLinesWhichMatchCurrentSourceLine(pData);
            executableLineCount++;
            if (pData->minCount)
            {
                executedLineCount++;
                fprintf(pData->pDestFile, "%10u: %s\n", pData->minCount, pSourceLine);
            }
            else
            {
                fprintf(pData->pDestFile, "     #####: %s\n", pSourceLine);
            }
        }
        else
        {
            fprintf(pData->pDestFile, "         -: %s\n", pSourceLine);
        }
        pData->currentSourceLine++;
    }
    assert( executableLineCount > 0 );
    pData->percentCovered = 100.0f * (float)executedLineCount / (float)executableLineCount;
}

static const char* getNextSourceLine(PrivateData* pData)
{
    char* pTextStart = pData->pCurr;
    char  previous = '\0';

    if (*pData->pCurr == '\0')
        return NULL;

    while (*pData->pCurr && *pData->pCurr != '\n' && *pData->pCurr != '\r')
        pData->pCurr++;

    previous = *pData->pCurr;
    if (*pData->pCurr != '\0')
        *pData->pCurr++ = '\0';
    if (isTwoCharacterLineTerminator(previous, *pData->pCurr))
        pData->pCurr++;
    return pTextStart;
}

static int isTwoCharacterLineTerminator(char previous, char current)
{
    return (previous == '\r' && current == '\n') || (previous == '\n' && current == '\r');
}

static int doesCurrentSourceLineMatchCurrentElfLine(PrivateData* pData)
{
    return  pData->currentElfLine < pData->pLines->lineCount &&
            pData->pLines->pLines[pData->currentElfLine].pFilename == pData->pSourceFilename &&
            pData->pLines->pLines[pData->currentElfLine].lineNumber == pData->currentSourceLine;
}

static void iterateOverElfLinesWhichMatchCurrentSourceLine(PrivateData* pData)
{
    pData->minCount = UINT_MAX;
    while (doesCurrentSourceLineMatchCurrentElfLine(pData))
    {
        uint32_t count = MemorySim_GetFlashReadCount(pData->pMemory, pData->pLines->pLines[pData->currentElfLine].address);
        if (count < pData->minCount)
            pData->minCount = count;
        pData->currentElfLine++;
    }
}

static void closeSourceAndDestSourceFiles(PrivateData* pData)
{
    fclose(pData->pSourceFile);
    fclose(pData->pDestFile);
    pData->pSourceFile = NULL;
    pData->pDestFile = NULL;
}

static void uninitPrivateData(PrivateData* pData)
{
    if (pData->pSummaryFile)
        fclose(pData->pSummaryFile);
    closeSourceAndDestSourceFiles(pData);
    ElfLines_Uninit(pData->pLines);
    free(pData->pOutputFilename);
    free(pData->pSourceFileText);
}


const char* CodeCoverage_GetErrorText(void)
{
    return g_errorText;
}

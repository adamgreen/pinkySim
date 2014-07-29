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

// Include headers from C modules under test.
extern "C"
{
    #include <CodeCoverage.h>
    #include <common.h>
    #include <FileFailureInject.h>
    #include <MallocFailureInject.h>
    #include <MemorySim.h>
    #include <mockFileIo.h>
}

// Include C++ headers for test harness.
#include "CppUTest/TestHarness.h"


TEST_GROUP(CodeCoverage)
{
    IMemory*    m_pMemory;
    char*       m_pBuffer;

    void setup()
    {
        static const uint32_t flashImage[] = { 0x10000008, 0, 0, 0,
                                               0,          0, 0, 0 };
        m_pMemory = MemorySim_Init();
        MemorySim_CreateRegionsFromFlashImage(m_pMemory, flashImage, sizeof(flashImage));
        m_pBuffer = NULL;
        cleanupFiles();
    }

    void cleanupFiles()
    {
        remove("summary.txt");
        remove("CodeCoverageTest1.S");
        remove("CodeCoverageTest2.S");
        remove("CodeCoverageTest1.S.cov");
        remove("CodeCoverageTest2.S.cov");
    }

    void teardown()
    {
        CHECK_EQUAL(0, getExceptionCode());
        clearExceptionCode();
        mockFileIo_Uninit();
        MemorySim_Uninit(m_pMemory);
        MallocFailureInject_Restore();
        freadRestore();
        free(m_pBuffer);
        cleanupFiles();
    }

    void validateExceptionThrown(int expectedExceptionCode)
    {
        CHECK_EQUAL(expectedExceptionCode, getExceptionCode());
        clearExceptionCode();
    }

    void createSourceFile(const char* pSourceFilename, const char* pText)
    {
        FILE* pFile = fopen(pSourceFilename, "w");
        size_t length = strlen(pText);
        fwrite(pText, 1, length, pFile);
        fclose(pFile);
    }

    void checkFileMatches(const char* pFilename, const char* pText)
    {
        FILE* pFile = fopen(pFilename, "r");
        CHECK(pFile != NULL);
        size_t length = strlen(pText);
        m_pBuffer = (char*)malloc(length + 1);
        int bytesRead = fread(m_pBuffer, 1, length + 1, pFile);
        m_pBuffer[length] = '\0';
        STRCMP_EQUAL(pText, m_pBuffer);
        CHECK_EQUAL((int)length, bytesRead);
        CHECK_TRUE(feof(pFile));
        fclose(pFile);
        free(m_pBuffer);
        m_pBuffer = NULL;
    }
};


TEST(CodeCoverage, FailElfParsing_ShouldThrow)
{
    mockFileIo_SetPOpenCallResult(NULL);
        __try_and_catch( CodeCoverage_Run("foo.elf", m_pMemory,  ".", NULL, 0) );
    validateExceptionThrown(fileException);
    STRCMP_EQUAL("error: Failed to parse line information for foo.elf.", CodeCoverage_GetErrorText());
}

TEST(CodeCoverage, EmptyElfLines_ShouldGenerateNoOutput)
{
    CodeCoverage_Run("foo.elf", m_pMemory, ".", NULL, 0);
    STRCMP_EQUAL("", CodeCoverage_GetErrorText());
    checkFileMatches("./summary.txt", "");
}

TEST(CodeCoverage, OneLineInElf_FailSourceFileOpen_ShouldThrow)
{
    const char* lines[] = { "CU: CodeCoverageTest1.S:\n",
                            "CodeCoverageTest1.S 1 0x4\n" };
    mockFileIo_SetFgetsData(lines, ARRAY_SIZE(lines));
        __try_and_catch( CodeCoverage_Run("foo.elf", m_pMemory, ".", NULL, 0) );
    validateExceptionThrown(fileException);
    STRCMP_EQUAL("error: Failed to open CodeCoverageTest1.S.", CodeCoverage_GetErrorText());
}

TEST(CodeCoverage, FailAllMemoryAllocations)
{
    const char* lines[] = { "CU: CodeCoverageTest1.S:\n",
                            "CodeCoverageTest1.S 1 0x4\n" };
    static const int allocationsToFail = 6;
    createSourceFile("CodeCoverageTest1.S", "Line 1");
    for (int i = 1 ; i <= allocationsToFail ; i++)
    {
        mockFileIo_SetFgetsData(lines, ARRAY_SIZE(lines));
        MallocFailureInject_FailAllocation(i);
            __try_and_catch( CodeCoverage_Run("foo.elf", m_pMemory, ".", NULL, 0) );
        validateExceptionThrown(outOfMemoryException);
    }

    mockFileIo_SetFgetsData(lines, ARRAY_SIZE(lines));
    MallocFailureInject_FailAllocation(allocationsToFail + 1);
        CodeCoverage_Run("foo.elf", m_pMemory, ".", NULL, 0);
    MallocFailureInject_Restore();
    checkFileMatches("./summary.txt", "  0.00%  CodeCoverageTest1.S\n");
    checkFileMatches("./CodeCoverageTest1.S.cov", "     #####: Line 1\n");
}

TEST(CodeCoverage, FailSourceFileRead)
{
    const char* lines[] = { "CU: CodeCoverageTest1.S:\n",
                            "CodeCoverageTest1.S 1 0x4\n" };

    mockFileIo_SetFgetsData(lines, ARRAY_SIZE(lines));
    createSourceFile("CodeCoverageTest1.S", "Line 1");
    freadFail(1);
        __try_and_catch( CodeCoverage_Run("foo.elf", m_pMemory, ".", NULL, 0) );
    validateExceptionThrown(fileException);
    STRCMP_EQUAL("error: Failed to read CodeCoverageTest1.S.", CodeCoverage_GetErrorText());
}

TEST(CodeCoverage, OneLineInElf_SingleLineSource_NotExecuted_VerifyOutputFiles)
{
    const char* lines[] = { "CU: CodeCoverageTest1.S:\n",
                            "CodeCoverageTest1.S 1 0x4\n" };

    mockFileIo_SetFgetsData(lines, ARRAY_SIZE(lines));
    createSourceFile("CodeCoverageTest1.S", "Line 1");
        CodeCoverage_Run("foo.elf", m_pMemory, ".", NULL, 0);
    checkFileMatches("./summary.txt", "  0.00%  CodeCoverageTest1.S\n");
    checkFileMatches("./CodeCoverageTest1.S.cov", "     #####: Line 1\n");
}

TEST(CodeCoverage, OneLineInElf_SingleLineSourceWithDirectoryName_NotExecuted_VerifyOutputFiles)
{
    const char* lines[] = { "CU: ./CodeCoverageTest1.S:\n",
                            "CodeCoverageTest1.S 1 0x4\n" };

    mockFileIo_SetFgetsData(lines, ARRAY_SIZE(lines));
    createSourceFile("CodeCoverageTest1.S", "Line 1");
        CodeCoverage_Run("foo.elf", m_pMemory, ".", NULL, 0);
    checkFileMatches("./summary.txt", "  0.00%  ./CodeCoverageTest1.S\n");
    checkFileMatches("./CodeCoverageTest1.S.cov", "     #####: Line 1\n");
}

TEST(CodeCoverage, TwoLinesInElf_MultiLineSource_NotExecuted_VerifyOutputFiles)
{
    const char* lines[] = { "CU: CodeCoverageTest1.S:\n",
                            "CodeCoverageTest1.S 1 0x4\n",
                            "\n",
                            "CodeCoverageTest1.S 2 0x8\n" };

    mockFileIo_SetFgetsData(lines, ARRAY_SIZE(lines));
    createSourceFile("CodeCoverageTest1.S", "Line 1\n"
                                            "Line 2\n");
        CodeCoverage_Run("foo.elf", m_pMemory, ".", NULL, 0);
    checkFileMatches("./summary.txt", "  0.00%  CodeCoverageTest1.S\n");
    checkFileMatches("./CodeCoverageTest1.S.cov", "     #####: Line 1\n"
                                                  "     #####: Line 2\n");
}

TEST(CodeCoverage, ThreeLinesInElf_MultiLineSourceWithBlankLine_NotExecuted_VerifyOutputFiles)
{
    const char* lines[] = { "CU: CodeCoverageTest1.S:\n",
                            "CodeCoverageTest1.S 1 0x4\n",
                            "\n",
                            "CodeCoverageTest1.S 2 0x8\n",
                            "\n",
                            "CodeCoverageTest1.S 3 0xc\n"};

    mockFileIo_SetFgetsData(lines, ARRAY_SIZE(lines));
    createSourceFile("CodeCoverageTest1.S", "Line 1\n"
                                            "\n"
                                            "Line 3\n");
        CodeCoverage_Run("foo.elf", m_pMemory, ".", NULL, 0);
    checkFileMatches("./summary.txt", "  0.00%  CodeCoverageTest1.S\n");
    checkFileMatches("./CodeCoverageTest1.S.cov", "     #####: Line 1\n"
                                                  "     #####: \n"
                                                  "     #####: Line 3\n");
}

TEST(CodeCoverage, TwoLinesInElf_MultiLineSourceWithWindowsLineEndings_NotExecuted_VerifyOutputFiles)
{
    const char* lines[] = { "CU: CodeCoverageTest1.S:\n",
                            "CodeCoverageTest1.S 1 0x4\n,"
                            "\n",
                            "CodeCoverageTest1.S 2 0x8\n" };

    mockFileIo_SetFgetsData(lines, ARRAY_SIZE(lines));
    createSourceFile("CodeCoverageTest1.S", "Line 1\r\n"
                                            "Line 2\r\n");
        CodeCoverage_Run("foo.elf", m_pMemory, ".", NULL, 0);
    checkFileMatches("./summary.txt", "  0.00%  CodeCoverageTest1.S\n");
    checkFileMatches("./CodeCoverageTest1.S.cov", "     #####: Line 1\n"
                                                  "     #####: Line 2\n");
}

TEST(CodeCoverage, TwoLinesInElf_MultiLineSourceWithOldMacLineEndings_NotExecuted_VerifyOutputFiles)
{
    const char* lines[] = { "CU: CodeCoverageTest1.S:\n",
                            "CodeCoverageTest1.S 1 0x4\n",
                            "\n",
                            "CodeCoverageTest1.S 2 0x8\n" };

    mockFileIo_SetFgetsData(lines, ARRAY_SIZE(lines));
    createSourceFile("CodeCoverageTest1.S", "Line 1\n\r"
                                            "Line 2\n\r");
        CodeCoverage_Run("foo.elf", m_pMemory, ".", NULL, 0);
    checkFileMatches("./summary.txt", "  0.00%  CodeCoverageTest1.S\n");
    checkFileMatches("./CodeCoverageTest1.S.cov", "     #####: Line 1\n"
                                                  "     #####: Line 2\n");
}

TEST(CodeCoverage, TwoLinesInElf_MultiLineSourceWithCarriageReturnLineEndings_NotExecutable_VerifyOutputFiles)
{
    const char* lines[] = { "CU: CodeCoverageTest1.S:\n",
                            "CodeCoverageTest1.S 1 0x4\n",
                            "\n",
                            "CodeCoverageTest1.S 2 0x8\n" };

    mockFileIo_SetFgetsData(lines, ARRAY_SIZE(lines));
    createSourceFile("CodeCoverageTest1.S", "Line 1\r"
                                            "Line 2\r");
        CodeCoverage_Run("foo.elf", m_pMemory, ".", NULL, 0);
    checkFileMatches("./summary.txt", "  0.00%  CodeCoverageTest1.S\n");
    checkFileMatches("./CodeCoverageTest1.S.cov", "     #####: Line 1\n"
                                                  "     #####: Line 2\n");
}

TEST(CodeCoverage, OneLineInElf_SingleLineSource_ExecutedOnce_VerifyOutputFiles)
{
    const char* lines[] = { "CU: CodeCoverageTest1.S:\n",
                            "CodeCoverageTest1.S 1 0x4\n" };

    mockFileIo_SetFgetsData(lines, ARRAY_SIZE(lines));
    IMemory_Read16(m_pMemory, 0x4);
    createSourceFile("CodeCoverageTest1.S", "Line 1");
        CodeCoverage_Run("foo.elf", m_pMemory, ".", NULL, 0);
    checkFileMatches("./summary.txt", "100.00%  CodeCoverageTest1.S\n");
    checkFileMatches("./CodeCoverageTest1.S.cov", "         1: Line 1\n");
}

TEST(CodeCoverage, OneLineInElf_SingleLineSource_ExecutedTwice_VerifyOutputFiles)
{
    const char* lines[] = { "CU: CodeCoverageTest1.S:\n",
                            "CodeCoverageTest1.S 1 0x4\n" };

    mockFileIo_SetFgetsData(lines, ARRAY_SIZE(lines));
    IMemory_Read16(m_pMemory, 0x4);
    IMemory_Read16(m_pMemory, 0x4);
    createSourceFile("CodeCoverageTest1.S", "Line 1");
        CodeCoverage_Run("foo.elf", m_pMemory, ".", NULL, 0);
    checkFileMatches("./summary.txt", "100.00%  CodeCoverageTest1.S\n");
    checkFileMatches("./CodeCoverageTest1.S.cov", "         2: Line 1\n");
}

TEST(CodeCoverage, TwoLinesInElf_FirstAddressExecutedMoreThanSecond_SingleLineSource_VerifyMinimumCountUsed)
{
    const char* lines[] = { "CU: CodeCoverageTest1.S:\n",
                            "CodeCoverageTest1.S 1 0x4\n",
                            "\n",
                            "CodeCoverageTest1.S 1 0x8\n" };

    mockFileIo_SetFgetsData(lines, ARRAY_SIZE(lines));
    IMemory_Read16(m_pMemory, 0x4);
    IMemory_Read16(m_pMemory, 0x4);
    IMemory_Read16(m_pMemory, 0x8);
    createSourceFile("CodeCoverageTest1.S", "Line 1");
        CodeCoverage_Run("foo.elf", m_pMemory, ".", NULL, 0);
    checkFileMatches("./summary.txt", "100.00%  CodeCoverageTest1.S\n");
    checkFileMatches("./CodeCoverageTest1.S.cov", "         1: Line 1\n");
}

TEST(CodeCoverage, TwoLinesInElf_FirstAddressExecutedLessThanSecond_SingleLineSource_VerifyMinimumCountUsed)
{
    const char* lines[] = { "CU: CodeCoverageTest1.S:\n",
                            "CodeCoverageTest1.S 1 0x4\n",
                            "\n",
                            "CodeCoverageTest1.S 1 0x8\n" };

    mockFileIo_SetFgetsData(lines, ARRAY_SIZE(lines));
    IMemory_Read16(m_pMemory, 0x4);
    IMemory_Read16(m_pMemory, 0x8);
    IMemory_Read16(m_pMemory, 0x8);
    createSourceFile("CodeCoverageTest1.S", "Line 1");
        CodeCoverage_Run("foo.elf", m_pMemory, ".", NULL, 0);
    checkFileMatches("./summary.txt", "100.00%  CodeCoverageTest1.S\n");
    checkFileMatches("./CodeCoverageTest1.S.cov", "         1: Line 1\n");
}

TEST(CodeCoverage, OneLineInElf_SingleLineSource_NotExecutable_VerifyOutputFiles)
{
    const char* lines[] = { "CU: CodeCoverageTest1.S:\n",
                            "CodeCoverageTest1.S 1 0x0\n" };

    mockFileIo_SetFgetsData(lines, ARRAY_SIZE(lines));
    createSourceFile("CodeCoverageTest1.S", "Line 1");
        CodeCoverage_Run("foo.elf", m_pMemory, ".", NULL, 0);
    checkFileMatches("./summary.txt", "");
    CHECK(NULL == fopen("./CodeCoverageTest1.S.cov", "r"));
}

TEST(CodeCoverage, TwoLinesInElf_MultiLineSource_Executed_NotExecuted_NonExecutable_VerifyOutputFiles)
{
    const char* lines[] = { "CU: CodeCoverageTest1.S:\n",
                            "CodeCoverageTest1.S 1 0x4\n",
                            "\n",
                            "CodeCoverageTest1.S 2 0x8\n" };

    mockFileIo_SetFgetsData(lines, ARRAY_SIZE(lines));
    IMemory_Read16(m_pMemory, 0x4);
    createSourceFile("CodeCoverageTest1.S", "Line 1\n"
                                            "Line 2\n"
                                            "Line 3\n");
        CodeCoverage_Run("foo.elf", m_pMemory, ".", NULL, 0);
    checkFileMatches("./summary.txt", " 50.00%  CodeCoverageTest1.S\n");
    checkFileMatches("./CodeCoverageTest1.S.cov", "         1: Line 1\n"
                                                  "     #####: Line 2\n"
                                                  "         -: Line 3\n");
}

TEST(CodeCoverage, TwoLinesInElf_TwoSourceFiles_NotExecuted_VerifyOutputFiles)
{
    const char* lines[] = { "CU: CodeCoverageTest1.S:\n",
                            "CodeCoverageTest1.S 1 0x4\n",
                            "\n",
                            "CU: CodeCoverageTest2.S:\n",
                            "CodeCoverageTest2.S 1 0x8\n" };

    mockFileIo_SetFgetsData(lines, ARRAY_SIZE(lines));
    createSourceFile("CodeCoverageTest1.S", "Line 1\n");
    createSourceFile("CodeCoverageTest2.S", "Line 1\n");
        CodeCoverage_Run("foo.elf", m_pMemory, ".", NULL, 0);
    checkFileMatches("./summary.txt", "  0.00%  CodeCoverageTest1.S\n"
                                      "  0.00%  CodeCoverageTest2.S\n");
    checkFileMatches("./CodeCoverageTest1.S.cov", "     #####: Line 1\n");
    checkFileMatches("./CodeCoverageTest2.S.cov", "     #####: Line 1\n");
}

TEST(CodeCoverage, TwoLinesInElf_TwoSourceFiles_Executed_VerifyOutputFiles)
{
    const char* lines[] = { "CU: CodeCoverageTest1.S:\n",
                            "CodeCoverageTest1.S 1 0x4\n",
                            "\n",
                            "CU: CodeCoverageTest2.S:\n",
                            "CodeCoverageTest2.S 1 0x8\n" };

    mockFileIo_SetFgetsData(lines, ARRAY_SIZE(lines));
    IMemory_Read16(m_pMemory, 0x4);
    IMemory_Read16(m_pMemory, 0x8);
    createSourceFile("CodeCoverageTest1.S", "Line 1\n");
    createSourceFile("CodeCoverageTest2.S", "Line 1\n");
        CodeCoverage_Run("foo.elf", m_pMemory, ".", NULL, 0);
    checkFileMatches("./summary.txt", "100.00%  CodeCoverageTest1.S\n"
                                      "100.00%  CodeCoverageTest2.S\n");
    checkFileMatches("./CodeCoverageTest1.S.cov", "         1: Line 1\n");
    checkFileMatches("./CodeCoverageTest2.S.cov", "         1: Line 1\n");
}

TEST(CodeCoverage, TwoLinesInElf_TwoSourceFiles_MixOfExecutedAndNotExecuted_VerifyOutputFiles)
{
    const char* lines[] = { "CU: CodeCoverageTest1.S:\n",
                            "CodeCoverageTest1.S 1 0x4\n",
                            "\n",
                            "CU: CodeCoverageTest2.S:\n",
                            "CodeCoverageTest2.S 1 0x8\n" };

    mockFileIo_SetFgetsData(lines, ARRAY_SIZE(lines));
    IMemory_Read16(m_pMemory, 0x8);
    createSourceFile("CodeCoverageTest1.S", "Line 1\n");
    createSourceFile("CodeCoverageTest2.S", "Line 1\n");
        CodeCoverage_Run("foo.elf", m_pMemory, ".", NULL, 0);
    checkFileMatches("./summary.txt", "  0.00%  CodeCoverageTest1.S\n"
                                      "100.00%  CodeCoverageTest2.S\n");
    checkFileMatches("./CodeCoverageTest1.S.cov", "     #####: Line 1\n");
    checkFileMatches("./CodeCoverageTest2.S.cov", "         1: Line 1\n");
}

TEST(CodeCoverage, RestrictToOnlyProcessOneSourceFileOfTwoPossibleFiles_VerifyOutputFiles)
{
    const char* lines[] = { "CU: CodeCoverageTest1.S:\n",
                            "CodeCoverageTest1.S 1 0x4\n",
                            "\n",
                            "CU: CodeCoverageTest2.S:\n",
                            "CodeCoverageTest2.S 1 0x8\n" };
    const char* restrict[] = { "CodeCoverageTest2.S" };

    mockFileIo_SetFgetsData(lines, ARRAY_SIZE(lines));
    IMemory_Read16(m_pMemory, 0x4);
    IMemory_Read16(m_pMemory, 0x8);
    createSourceFile("CodeCoverageTest1.S", "Line 1\n");
    createSourceFile("CodeCoverageTest2.S", "Line 1\n");
        CodeCoverage_Run("foo.elf", m_pMemory, ".", restrict, ARRAY_SIZE(restrict));
    checkFileMatches("./summary.txt", "100.00%  CodeCoverageTest2.S\n");
    CHECK(NULL == fopen("./CodeCoverageTest1.S.cov", "r"));
    checkFileMatches("./CodeCoverageTest2.S.cov", "         1: Line 1\n");
}

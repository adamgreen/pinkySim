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
    #include <common.h>
    #include <ElfLines.h>
    #include <MallocFailureInject.h>
    #include <mockFileIo.h>
}

// Include C++ headers for test harness.
#include "CppUTest/TestHarness.h"


TEST_GROUP(ElfLines)
{
    FILE      m_file;
    ElfLines* m_pLines;

    void setup()
    {
        // Fake out the file handle returned from popen().  It will only be passed into mocks so it doesn't matter that
        // it isn't a real file pointer.  It just doesn't want to be a NULL pointer.
        mockFileIo_SetPOpenCallResult(&m_file);
        mockFileIo_SetFEOFCallResult(1);
        m_pLines = NULL;
    }

    void teardown()
    {
        CHECK_EQUAL(noException, getExceptionCode());
        clearExceptionCode();
        ElfLines_Uninit(m_pLines);
        MallocFailureInject_Restore();
        mockFileIo_Uninit();
    }

    void validateExceptionThrown(int expectedExceptionCode)
    {
        CHECK_EQUAL(expectedExceptionCode, getExceptionCode());
        clearExceptionCode();
    }
};


TEST(ElfLines, FailMemoryAllocations_ShouldThrow)
{
    static const int allocationsToFail = 4;
    for (int i = 1 ; i <= allocationsToFail ; i++)
    {
        const char* lines[] = { "CU: FileTest/main.c:\n",
                                "main.c                                        20                0xbc" };
        mockFileIo_SetFgetsData(lines, ARRAY_SIZE(lines));
        MallocFailureInject_FailAllocation(i);
            __try_and_catch( ElfLines_Parse("foo.elf") );
        validateExceptionThrown(outOfMemoryException);
    }
    MallocFailureInject_FailAllocation(allocationsToFail + 1);
    m_pLines = ElfLines_Parse("foo.elf");
    CHECK(NULL != m_pLines);
}

TEST(ElfLines, FailPOpenCall_ShouldThrow)
{
    mockFileIo_SetPOpenCallResult(NULL);
        __try_and_catch( ElfLines_Parse("foo.elf") );
    validateExceptionThrown(fileException);
}

TEST(ElfLines, FailFgets_NotEOF_ShouldThrow)
{
    mockFileIo_SetFgetsData(NULL, 0);
    mockFileIo_SetFEOFCallResult(0);
        __try_and_catch( ElfLines_Parse("foo.elf") );
    validateExceptionThrown(fileException);
}

TEST(ElfLines, ProcessOneValidLine)
{
    const char* lines[] = { "CU: FileTest/main.c:\n",
                            "main.c                                        20                0xbc" };
    mockFileIo_SetFgetsData(lines, ARRAY_SIZE(lines));
        m_pLines = ElfLines_Parse("foo.elf");
    CHECK_EQUAL(1, m_pLines->lineCount);
    STRCMP_EQUAL("FileTest/main.c", m_pLines->pLines[0].pFilename);
    CHECK_EQUAL(0xbc, m_pLines->pLines[0].address);
    CHECK_EQUAL(20, m_pLines->pLines[0].lineNumber);
}

TEST(ElfLines, ProcessDifferentFilename)
{
    const char* lines[] = { "CU: FileTest/foobar.c:\n",
                            "foobar.c                                      20                0xbc" };
    mockFileIo_SetFgetsData(lines, ARRAY_SIZE(lines));
        m_pLines = ElfLines_Parse("foo.elf");
    CHECK_EQUAL(1, m_pLines->lineCount);
    STRCMP_EQUAL("FileTest/foobar.c", m_pLines->pLines[0].pFilename);
    CHECK_EQUAL(0xbc, m_pLines->pLines[0].address);
    CHECK_EQUAL(20, m_pLines->pLines[0].lineNumber);
}

TEST(ElfLines, ProcessDifferentLineNumber)
{
    const char* lines[] = { "CU: FileTest/main.c:\n",
                            "main.c                                         1                0xbc" };
    mockFileIo_SetFgetsData(lines, ARRAY_SIZE(lines));
        m_pLines = ElfLines_Parse("foo.elf");
    CHECK_EQUAL(1, m_pLines->lineCount);
    STRCMP_EQUAL("FileTest/main.c", m_pLines->pLines[0].pFilename);
    CHECK_EQUAL(0xbc, m_pLines->pLines[0].address);
    CHECK_EQUAL(1, m_pLines->pLines[0].lineNumber);
}

TEST(ElfLines, ProcessDifferentAddress)
{
    const char* lines[] = { "CU: FileTest/main.c:\n",
                            "main.c                                        20                0xbaadf00d" };
    mockFileIo_SetFgetsData(lines, ARRAY_SIZE(lines));
        m_pLines = ElfLines_Parse("foo.elf");
    CHECK_EQUAL(1, m_pLines->lineCount);
    STRCMP_EQUAL("FileTest/main.c", m_pLines->pLines[0].pFilename);
    CHECK_EQUAL(0xbaadf00d, m_pLines->pLines[0].address);
    CHECK_EQUAL(20, m_pLines->pLines[0].lineNumber);
}

TEST(ElfLines, ProcessOneValidLine_FullFilenameHasNoDirectoryComponent)
{
    const char* lines[] = { "CU: main.c:\n",
                            "main.c                                        20                0xbc" };
    mockFileIo_SetFgetsData(lines, ARRAY_SIZE(lines));
        m_pLines = ElfLines_Parse("foo.elf");
    STRCMP_EQUAL("main.c", m_pLines->pLines[0].pFilename);
    CHECK_EQUAL(0xbc, m_pLines->pLines[0].address);
    CHECK_EQUAL(20, m_pLines->pLines[0].lineNumber);
}

TEST(ElfLines, ProcessOneLineToBeDiscarded)
{
    const char* lines[] = { "CU: FileTest/main.c:\n",
                            "main.c                                        20                0x0" };
    mockFileIo_SetFgetsData(lines, ARRAY_SIZE(lines));
        m_pLines = ElfLines_Parse("foo.elf");
    CHECK_EQUAL(0, m_pLines->lineCount);
}

TEST(ElfLines, ProcessTwoValidLines_ContainsTypicalExtraHeadersAndBlankLines)
{
    const char* lines[] = { "Decoded dump of debug contents of section .debug_line:\n",
                            "\n",
                            "CU: FileTest/main.c:\n",
                            "File name                            Line number    Starting address\n",
                            "main.c                                        20                0xbc\n",
                            "\n",
                            "main.c                                        26                0xc4\n" };
    mockFileIo_SetFgetsData(lines, ARRAY_SIZE(lines));
        m_pLines = ElfLines_Parse("foo.elf");
    CHECK_EQUAL(2, m_pLines->lineCount);
    STRCMP_EQUAL("FileTest/main.c", m_pLines->pLines[0].pFilename);
    CHECK_EQUAL(0xbc, m_pLines->pLines[0].address);
    CHECK_EQUAL(20, m_pLines->pLines[0].lineNumber);
    STRCMP_EQUAL("FileTest/main.c", m_pLines->pLines[1].pFilename);
    CHECK_EQUAL(0xc4, m_pLines->pLines[1].address);
    CHECK_EQUAL(26, m_pLines->pLines[1].lineNumber);
}

TEST(ElfLines, ProcessTwoLinesToBeDiscarded)
{
    const char* lines[] = { "CU: libstartup/NewlibRetarget.c:\n",
                            "NewlibRetarget.c                              37                   0\n",
                            "\n",
                            "NewlibRetarget.c                              38                 0x2\n" };
    mockFileIo_SetFgetsData(lines, ARRAY_SIZE(lines));
        m_pLines = ElfLines_Parse("foo.elf");
    CHECK_EQUAL(0, m_pLines->lineCount);
}

TEST(ElfLines, ProcessTwoLinesToBeDiscarded_TransitionToNewFile)
{
    const char* lines[] = { "CU: libstartup/NewlibRetarget.c:\n",
                            "NewlibRetarget.c                              37                   0\n",
                            "\n",
                            "NewlibRetarget.c                              38                 0x2\n",
                            "CU: test/foobar.c:\n" };
    mockFileIo_SetFgetsData(lines, ARRAY_SIZE(lines));
        m_pLines = ElfLines_Parse("foo.elf");
    CHECK_EQUAL(0, m_pLines->lineCount);
}

TEST(ElfLines, ProcessTwoFiles_TwoLinesPerFile)
{
    const char* lines[] = { "CU: FileTest/main.c:\n",
                            "main.c                                        20                0xbc\n",
                            "\n",
                            "main.c                                        26                0xc4\n",
                            "\n",
                            "CU: libstartup/NewlibRetarget.c:\n",
                            "NewlibRetarget.c                              32               0x220\n",
                            "\n",
                            "NewlibRetarget.c                              33               0x222\n" };
    mockFileIo_SetFgetsData(lines, ARRAY_SIZE(lines));
        m_pLines = ElfLines_Parse("foo.elf");
    CHECK_EQUAL(4, m_pLines->lineCount);
    STRCMP_EQUAL("FileTest/main.c", m_pLines->pLines[0].pFilename);
    CHECK_EQUAL(0xbc, m_pLines->pLines[0].address);
    CHECK_EQUAL(20, m_pLines->pLines[0].lineNumber);
    STRCMP_EQUAL("FileTest/main.c", m_pLines->pLines[1].pFilename);
    CHECK_EQUAL(0xc4, m_pLines->pLines[1].address);
    CHECK_EQUAL(26, m_pLines->pLines[1].lineNumber);

    STRCMP_EQUAL("libstartup/NewlibRetarget.c", m_pLines->pLines[2].pFilename);
    CHECK_EQUAL(0x220, m_pLines->pLines[2].address);
    CHECK_EQUAL(32, m_pLines->pLines[2].lineNumber);
    STRCMP_EQUAL("libstartup/NewlibRetarget.c", m_pLines->pLines[3].pFilename);
    CHECK_EQUAL(0x222, m_pLines->pLines[3].address);
    CHECK_EQUAL(33, m_pLines->pLines[3].lineNumber);
}

TEST(ElfLines, InterleaveDiscardedAndValidLinesTogether)
{
    const char* lines[] = { "CU: libstartup/NewlibRetarget.c:\n",
                            "File name                            Line number    Starting address\n",
                            "NewlibRetarget.c                              32               0x220\n",
                            "\n",
                            "NewlibRetarget.c                              33               0x222\n",
                            "NewlibRetarget.c                              37                   0\n",
                            "\n",
                            "NewlibRetarget.c                              38                 0x2\n",
                            "NewlibRetarget.c                              39                 0x6\n",
                            "NewlibRetarget.c                              42               0x228\n",
                            "\n",
                            "NewlibRetarget.c                              43               0x22a\n" };
    mockFileIo_SetFgetsData(lines, ARRAY_SIZE(lines));
        m_pLines = ElfLines_Parse("foo.elf");
    CHECK_EQUAL(4, m_pLines->lineCount);
    STRCMP_EQUAL("libstartup/NewlibRetarget.c", m_pLines->pLines[0].pFilename);
    CHECK_EQUAL(0x220, m_pLines->pLines[0].address);
    CHECK_EQUAL(32, m_pLines->pLines[0].lineNumber);
    STRCMP_EQUAL("libstartup/NewlibRetarget.c", m_pLines->pLines[1].pFilename);
    CHECK_EQUAL(0x222, m_pLines->pLines[1].address);
    CHECK_EQUAL(33, m_pLines->pLines[1].lineNumber);

    STRCMP_EQUAL("libstartup/NewlibRetarget.c", m_pLines->pLines[2].pFilename);
    CHECK_EQUAL(0x228, m_pLines->pLines[2].address);
    CHECK_EQUAL(42, m_pLines->pLines[2].lineNumber);
    STRCMP_EQUAL("libstartup/NewlibRetarget.c", m_pLines->pLines[3].pFilename);
    CHECK_EQUAL(0x22a, m_pLines->pLines[3].address);
    CHECK_EQUAL(43, m_pLines->pLines[3].lineNumber);
}

TEST(ElfLines, ProcessTwoFiles_TwoLinesPerFile_InputIsDescending_OutputAscending)
{
    const char* lines[] = { "CU: b.c:\n",
                            "b.c                                        33               0x222\n",
                            "\n",
                            "b.c                                        32               0x220\n",
                            "\n",
                            "CU: a.c:\n",
                            "a.c                                        26                0xc4\n",
                            "\n",
                            "a.c                                        20                0xbc\n" };
    mockFileIo_SetFgetsData(lines, ARRAY_SIZE(lines));
        m_pLines = ElfLines_Parse("foo.elf");
    CHECK_EQUAL(4, m_pLines->lineCount);
    STRCMP_EQUAL("a.c", m_pLines->pLines[0].pFilename);
    CHECK_EQUAL(0xbc, m_pLines->pLines[0].address);
    CHECK_EQUAL(20, m_pLines->pLines[0].lineNumber);
    STRCMP_EQUAL("a.c", m_pLines->pLines[1].pFilename);
    CHECK_EQUAL(0xc4, m_pLines->pLines[1].address);
    CHECK_EQUAL(26, m_pLines->pLines[1].lineNumber);

    STRCMP_EQUAL("b.c", m_pLines->pLines[2].pFilename);
    CHECK_EQUAL(0x220, m_pLines->pLines[2].address);
    CHECK_EQUAL(32, m_pLines->pLines[2].lineNumber);
    STRCMP_EQUAL("b.c", m_pLines->pLines[3].pFilename);
    CHECK_EQUAL(0x222, m_pLines->pLines[3].address);
    CHECK_EQUAL(33, m_pLines->pLines[3].lineNumber);
}

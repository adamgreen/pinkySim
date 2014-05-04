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
#include "string.h"

// Include headers from C modules under test.
extern "C"
{
    #include <FileFailureInject.h>
    #include <MallocFailureInject.h>
    #include <pinkySimCommandLine.h>
    #include <printfSpy.h>
    #include <SocketIComm.h>
}

// Include C++ headers for test harness.
#include "CppUTest/TestHarness.h"


#define ARRAY_SIZE(X) (sizeof(X)/sizeof(X[0]))


static const char     g_usageString[] = "Usage:";
static const char*    g_imageFilename = "image.bin";
static const uint32_t g_imageData[2] = { 0x10000004, 0x00000100 };


TEST_GROUP(pinkySimCommandLine)
{
    const char*         m_argv[10];
    pinkySimCommandLine m_commandLine;
    int                 m_argc;
    
    void setup()
    {
        memset(m_argv, 0, sizeof(m_argv));
        memset(&m_commandLine, 0xff, sizeof(m_commandLine));
        m_argc = 0;

        printfSpy_Hook(strlen(g_usageString));
    }

    void teardown()
    {
        CHECK_EQUAL(noException, getExceptionCode());
        fopenRestore();
        fseekRestore();
        ftellRestore();
        freadRestore();
        fwriteRestore();
        printfSpy_Unhook();
        MallocFailureInject_Restore();
        pinkySimCommandLine_Uninit(&m_commandLine);
        remove(g_imageFilename);
    }

    void addArg(const char* pArg)
    {
        CHECK(m_argc < (int)ARRAY_SIZE(m_argv));
        m_argv[m_argc++] = pArg;
    }
    
    void validateParamsAndNoErrorMessage(const char* pImageFilename,
                                         int breakOnStart = 0,
                                         uint16_t gdbPort = SOCKET_ICOMM_DEFAULT_PORT)
    {
        STRCMP_EQUAL("", printfSpy_GetLastOutput());
        STRCMP_EQUAL(pImageFilename, m_commandLine.pImageFilename);
        CHECK(m_commandLine.pMemory != NULL);
        CHECK_EQUAL(breakOnStart, m_commandLine.breakOnStart);
        CHECK_EQUAL(gdbPort, m_commandLine.gdbPort);
    }

    void validateExceptionThrownAndUsageStringDisplayed(int expectedException = invalidArgumentException)
    {
        CHECK_EQUAL(expectedException, getExceptionCode());
        STRCMP_EQUAL(g_usageString, printfSpy_GetLastOutput());
        clearExceptionCode();
    }
    
    void createTestImageFile()
    {
        FILE* pFile = fopen(g_imageFilename, "w");
        fwrite(g_imageData, 1, sizeof(g_imageData), pFile);
        fclose(pFile);
    }
};


TEST(pinkySimCommandLine, NoParameters)
{
    __try_and_catch( pinkySimCommandLine_Init(&m_commandLine, m_argc, m_argv) );
    validateExceptionThrownAndUsageStringDisplayed();
    CHECK(m_commandLine.pImageFilename == NULL);
    CHECK(m_commandLine.pMemory == NULL);
    CHECK_FALSE(m_commandLine.breakOnStart);
    CHECK_EQUAL(SOCKET_ICOMM_DEFAULT_PORT, m_commandLine.gdbPort);
}

TEST(pinkySimCommandLine, OneImageFilename_CheckRegions)
{
    addArg(g_imageFilename);
    createTestImageFile();
        pinkySimCommandLine_Init(&m_commandLine, m_argc, m_argv);
    validateParamsAndNoErrorMessage(g_imageFilename);
    CHECK_EQUAL(g_imageData[0], IMemory_Read32(m_commandLine.pMemory, 0x00000000));
    CHECK_EQUAL(g_imageData[1], IMemory_Read32(m_commandLine.pMemory, 0x00000004));
    CHECK_EQUAL(0, IMemory_Read32(m_commandLine.pMemory, 0x10000000));
}

TEST(pinkySimCommandLine, FailImageFileOpen)
{
    addArg(g_imageFilename);
    createTestImageFile();
    fopenFail(NULL);
        __try_and_catch( pinkySimCommandLine_Init(&m_commandLine, m_argc, m_argv) );
    validateExceptionThrownAndUsageStringDisplayed(fileException);
}

TEST(pinkySimCommandLine, FailImageFileSeek)
{
    addArg(g_imageFilename);
    createTestImageFile();
    fseekSetFailureCode(-1);
        __try_and_catch( pinkySimCommandLine_Init(&m_commandLine, m_argc, m_argv) );
    validateExceptionThrownAndUsageStringDisplayed(fileException);
}

TEST(pinkySimCommandLine, FailImageFileTell)
{
    addArg(g_imageFilename);
    createTestImageFile();
    ftellFail(-1);
        __try_and_catch( pinkySimCommandLine_Init(&m_commandLine, m_argc, m_argv) );
    validateExceptionThrownAndUsageStringDisplayed(fileException);
}

TEST(pinkySimCommandLine, FailSecondImageFileSeek)
{
    addArg(g_imageFilename);
    createTestImageFile();
    fseekSetFailureCode(-1);
    fseekSetCallsBeforeFailure(1);
        __try_and_catch( pinkySimCommandLine_Init(&m_commandLine, m_argc, m_argv) );
    validateExceptionThrownAndUsageStringDisplayed(fileException);
}

TEST(pinkySimCommandLine, FailImageFileBufferAllocation)
{
    addArg(g_imageFilename);
    createTestImageFile();
    MallocFailureInject_FailAllocation(1);    
        __try_and_catch( pinkySimCommandLine_Init(&m_commandLine, m_argc, m_argv) );
    validateExceptionThrownAndUsageStringDisplayed(outOfMemoryException);
}

TEST(pinkySimCommandLine, FailImageFileRead)
{
    addArg(g_imageFilename);
    createTestImageFile();
    freadFail(-1);
        __try_and_catch( pinkySimCommandLine_Init(&m_commandLine, m_argc, m_argv) );
    validateExceptionThrownAndUsageStringDisplayed(fileException);
}

TEST(pinkySimCommandLine, FailMemoryRegionAllocation)
{
    addArg(g_imageFilename);
    createTestImageFile();
    MallocFailureInject_FailAllocation(2);
        __try_and_catch( pinkySimCommandLine_Init(&m_commandLine, m_argc, m_argv) );
    validateExceptionThrownAndUsageStringDisplayed(outOfMemoryException);
}

TEST(pinkySimCommandLine, FlashOptionWithTwoMissingParams)
{
    addArg("--flash");
        __try_and_catch( pinkySimCommandLine_Init(&m_commandLine, m_argc, m_argv) );
    validateExceptionThrownAndUsageStringDisplayed();
}

TEST(pinkySimCommandLine, FlashOptionWithOneMissingParams)
{
    addArg("--flash");
    addArg("0x00000000");
        __try_and_catch( pinkySimCommandLine_Init(&m_commandLine, m_argc, m_argv) );
    validateExceptionThrownAndUsageStringDisplayed();
}

TEST(pinkySimCommandLine, FlashOptionValid_ButNoImageFilename_ShouldCleanupMemorySim)
{
    addArg("--flash");
    addArg("0x00000000");
    addArg("8");
        __try_and_catch( pinkySimCommandLine_Init(&m_commandLine, m_argc, m_argv) );
    validateExceptionThrownAndUsageStringDisplayed();
    CHECK(NULL == m_commandLine.pMemory);
}

TEST(pinkySimCommandLine, FlashOption_ShouldCreateValidReadOnlyMemoryRegion)
{
    addArg("--flash");
    addArg("0x00000000");
    addArg("8");
    addArg(g_imageFilename);
    createTestImageFile();
        pinkySimCommandLine_Init(&m_commandLine, m_argc, m_argv);
    validateParamsAndNoErrorMessage(g_imageFilename);
    IMemory_Read32(m_commandLine.pMemory, 0);
    IMemory_Read32(m_commandLine.pMemory, 4);
    __try_and_catch( IMemory_Read32(m_commandLine.pMemory, 8) );
    CHECK_EQUAL(busErrorException, getExceptionCode());
    __try_and_catch( IMemory_Write32(m_commandLine.pMemory, 0, 0) );
    CHECK_EQUAL(busErrorException, getExceptionCode());
    clearExceptionCode();
}

TEST(pinkySimCommandLine, FlashOption_FailMemoryAllocation_ShouldThrow)
{
    addArg("--flash");
    addArg("0x00000000");
    addArg("8");
    addArg(g_imageFilename);
    MallocFailureInject_FailAllocation(1);
        __try_and_catch( pinkySimCommandLine_Init(&m_commandLine, m_argc, m_argv) );
    validateExceptionThrownAndUsageStringDisplayed(outOfMemoryException);
}

TEST(pinkySimCommandLine, RamOption_ShouldCreateValidReadWriteMemoryRegion)
{
    addArg("--ram");
    addArg("0x10000000");
    addArg("4");
    addArg("--flash");
    addArg("0x00000000");
    addArg("8");
    addArg(g_imageFilename);
    createTestImageFile();
        pinkySimCommandLine_Init(&m_commandLine, m_argc, m_argv);
    validateParamsAndNoErrorMessage(g_imageFilename);

    CHECK_EQUAL(0, IMemory_Read32(m_commandLine.pMemory, 0x10000000));
    __try_and_catch( IMemory_Read32(m_commandLine.pMemory, 0x10000004) );
    CHECK_EQUAL(busErrorException, getExceptionCode());
    clearExceptionCode();
    IMemory_Write32(m_commandLine.pMemory, 0x10000000, 0xFFFFFFFF);
}

TEST(pinkySimCommandLine, RamOption_FailMemoryAllocation_ShouldThrow)
{
    addArg("--ram");
    addArg("0x00000000");
    addArg("8");
    addArg(g_imageFilename);
    MallocFailureInject_FailAllocation(1);
        __try_and_catch( pinkySimCommandLine_Init(&m_commandLine, m_argc, m_argv) );
    validateExceptionThrownAndUsageStringDisplayed(outOfMemoryException);
}

TEST(pinkySimCommandLine, RamOptionWithTwoMissingParams)
{
    addArg("--ram");
        __try_and_catch( pinkySimCommandLine_Init(&m_commandLine, m_argc, m_argv) );
    validateExceptionThrownAndUsageStringDisplayed();
}

TEST(pinkySimCommandLine, RamOptionWithOneMissingParams)
{
    addArg("--ram");
    addArg("0x00000000");
        __try_and_catch( pinkySimCommandLine_Init(&m_commandLine, m_argc, m_argv) );
    validateExceptionThrownAndUsageStringDisplayed();
}

TEST(pinkySimCommandLine, RamOptionValid_ButNoImageFilename_ShouldCleanupMemorySim)
{
    addArg("--ram");
    addArg("0x10000000");
    addArg("4");
        __try_and_catch( pinkySimCommandLine_Init(&m_commandLine, m_argc, m_argv) );
    validateExceptionThrownAndUsageStringDisplayed();
    CHECK(NULL == m_commandLine.pMemory);
}

TEST(pinkySimCommandLine, SetBreakOnStartFlag)
{
    addArg("--breakOnStart");
    addArg(g_imageFilename);
    createTestImageFile();
        pinkySimCommandLine_Init(&m_commandLine, m_argc, m_argv);
    validateParamsAndNoErrorMessage(g_imageFilename, 1);
}

TEST(pinkySimCommandLine, SetGdbPort)
{
    addArg("--gdbPort");
    addArg("6666");
    addArg(g_imageFilename);
    createTestImageFile();
        pinkySimCommandLine_Init(&m_commandLine, m_argc, m_argv);
    validateParamsAndNoErrorMessage(g_imageFilename, 0, 6666);
}

TEST(pinkySimCommandLine, SetGdbPort_FailWithTooFewParams)
{
    addArg("--gdbPort");
        __try_and_catch( pinkySimCommandLine_Init(&m_commandLine, m_argc, m_argv) );
    validateExceptionThrownAndUsageStringDisplayed();
}

TEST(pinkySimCommandLine, SetGdbPort_FailWithPortNumberTooHigh)
{
    addArg("--gdbPort");
    addArg("0x10000");
    addArg(g_imageFilename);
    createTestImageFile();
        __try_and_catch( pinkySimCommandLine_Init(&m_commandLine, m_argc, m_argv) );
    validateExceptionThrownAndUsageStringDisplayed();
}

TEST(pinkySimCommandLine, InvalidOption_ShouldThrow)
{
    addArg("--invalidOption");
        __try_and_catch( pinkySimCommandLine_Init(&m_commandLine, m_argc, m_argv) );
    validateExceptionThrownAndUsageStringDisplayed();
}

TEST(pinkySimCommandLine, AttemptToSpecifyImageFilenameTwice_ShouldThrow)
{
    addArg(g_imageFilename);
    addArg(g_imageFilename);
    createTestImageFile();
        __try_and_catch( pinkySimCommandLine_Init(&m_commandLine, m_argc, m_argv) );
    validateExceptionThrownAndUsageStringDisplayed();
}

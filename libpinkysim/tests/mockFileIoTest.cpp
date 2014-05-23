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
// Include headers from C modules under test.
extern "C"
{
    #include <mockFileIo.h>
}

// Include C++ headers for test harness.
#include "CppUTest/TestHarness.h"


TEST_GROUP(mockFileIo)
{
    void setup()
    {
    }

    void teardown()
    {
        mockFileIo_Uninit();
    }
};


TEST(mockFileIo, ReadStdIn_SetupNoDataToRead_ShouldReturnZero)
{
    char buffer[1];
    ssize_t result = read(STDIN_FILENO, buffer, sizeof(buffer));
    CHECK_EQUAL(0, result);
}

TEST(mockFileIo, ReadRegularFile_ShouldReturnNegativeOne)
{
    char buffer[1];
    ssize_t result = read(4, buffer, sizeof(buffer));
    CHECK_EQUAL(-1, result);
    CHECK_EQUAL(EBADF, errno);
}

TEST(mockFileIo, ReadTwoBytes_SetupOneByteOfData_ShouldReturnOneByte)
{
    char buffer[2] = {0xFF, 0xFF};
    mockFileIo_SetReadData("a", 1);
    ssize_t result = read(STDIN_FILENO, buffer, sizeof(buffer));
    CHECK_EQUAL(1, result);
    CHECK_EQUAL('a', buffer[0]);
    CHECK_EQUAL((char)0xFF, buffer[1]);
}

TEST(mockFileIo, ReadTwoBytes_SetupTwoBytesOfData_ShouldReturnTwoBytes)
{
    char buffer[2];
    mockFileIo_SetReadData("xy", 2);
    ssize_t result = read(STDIN_FILENO, buffer, sizeof(buffer));
    CHECK_EQUAL(2, result);
    CHECK_EQUAL('x', buffer[0]);
    CHECK_EQUAL('y', buffer[1]);
}

TEST(mockFileIo, Issue2Reads_FirstReadWillProcessOneByteOfData_SecondReadWillProcessRestOfData)
{
    char buffer[2] = {0, 0};
    mockFileIo_SetReadData("xy", 2);
    ssize_t result;
        result = read(STDIN_FILENO, buffer, 1);
    CHECK_EQUAL(1, result);
    CHECK_EQUAL('x', buffer[0]);
    CHECK_EQUAL('\0', buffer[1]);
        result = read(STDIN_FILENO, buffer + 1, 1);
    CHECK_EQUAL(1, result);
    CHECK_EQUAL('x', buffer[0]);
    CHECK_EQUAL('y', buffer[1]);
}

TEST(mockFileIo, FailReadCall)
{
    char buffer[2] = {0, 0};
    mockFileIo_SetReadToFail(-1, EFAULT);
    ssize_t result = read(STDIN_FILENO, buffer, 1);
    CHECK_EQUAL(-1, result);
    CHECK_EQUAL(EFAULT, errno);
}

TEST(mockFileIo, WriteStdOut_SetupNoOutputBuffer_ShouldReturnZero)
{
    ssize_t result = write(STDOUT_FILENO, "a", 1);
    CHECK_EQUAL(0, result);
}

TEST(mockFileIo, WriteRegularFile_ShouldReturnNegativeOne)
{
    ssize_t result = write(4, "a", 1);
    CHECK_EQUAL(-1, result);
    CHECK_EQUAL(EBADF, errno);
}

TEST(mockFileIo, WriteStdOut_SetupOneByteOutputBuffer_WriteOneByte)
{
    mockFileIo_CreateWriteBuffer(1);
    ssize_t result = write(STDOUT_FILENO, "a", 1);
    CHECK_EQUAL(1, result);
    STRCMP_EQUAL("a", mockFileIo_GetStdOutData());
}

TEST(mockFileIo, WriteStdErr_SetupOneByteOutputBuffer_WriteOneByte)
{
    mockFileIo_CreateWriteBuffer(1);
    ssize_t result = write(STDERR_FILENO, "a", 1);
    CHECK_EQUAL(1, result);
    STRCMP_EQUAL("a", mockFileIo_GetStdErrData());
}

TEST(mockFileIo, WriteStdOut_SetupOneByteOutputBuffer_AttemptToWriteTwoBytes)
{
    mockFileIo_CreateWriteBuffer(1);
    ssize_t result = write(STDOUT_FILENO, "xy", 2);
    CHECK_EQUAL(1, result);
    STRCMP_EQUAL("x", mockFileIo_GetStdOutData());
}

TEST(mockFileIo, WriteStdOut_SetupTwoByteOutputBuffer_WriteOneByteTwice)
{
    mockFileIo_CreateWriteBuffer(2);
    ssize_t result = write(STDOUT_FILENO, "y", 1);
    CHECK_EQUAL(1, result);
    STRCMP_EQUAL("y", mockFileIo_GetStdOutData());

    result = write(STDOUT_FILENO, "z", 1);
    CHECK_EQUAL(1, result);
    STRCMP_EQUAL("yz", mockFileIo_GetStdOutData());
}

TEST(mockFileIo, WriteToBothStdOutAndStdErr_ShouldLogSeparately)
{
    mockFileIo_CreateWriteBuffer(5);
    ssize_t result = write(STDOUT_FILENO, "Test1", 5);
    CHECK_EQUAL(5, result);
    STRCMP_EQUAL("Test1", mockFileIo_GetStdOutData());

    result = write(STDERR_FILENO, "Test2", 5);
    CHECK_EQUAL(5, result);
    STRCMP_EQUAL("Test1", mockFileIo_GetStdOutData());
    STRCMP_EQUAL("Test2", mockFileIo_GetStdErrData());
}

TEST(mockFileIo, SetOpenCallToFail)
{
    mockFileIo_SetOpenToFail(-1, EIO);
    int result = open("mockFileTest.tst", O_CREAT | O_TRUNC | O_RDWR);
    CHECK_EQUAL(-1, result);
    CHECK_EQUAL(EIO, errno);
}

TEST(mockFileIo, OpenCall_ReturnSuccess_ErrnoNotSet)
{
    mockFileIo_SetOpenToFail(3, EIO);
    errno = 0;
    int result = open("mockFileTest.tst", O_CREAT | O_TRUNC | O_RDWR);
    CHECK_EQUAL(3, result);
    CHECK_EQUAL(0, errno);
}

TEST(mockFileIo, SetLSeekCallToFail)
{
    mockFileIo_SetLSeekToFail(-1, EIO);
    off_t result = lseek(3, 0, SEEK_SET);
    CHECK_EQUAL(-1, (long)result);
    CHECK_EQUAL(EIO, errno);
}

TEST(mockFileIo, LSeekCall_ReturnSuccess_ErrnoNotSet)
{
    mockFileIo_SetLSeekToFail(0, EIO);
    errno = 0;
    off_t result = lseek(3, 0, SEEK_SET);
    CHECK_EQUAL(0, (long)result);
    CHECK_EQUAL(0, errno);
}

TEST(mockFileIo, SetCloseCallToFail)
{
    mockFileIo_SetCloseToFail(-1, EIO);
    int result = close(3);
    CHECK_EQUAL(-1, result);
    CHECK_EQUAL(EIO, errno);
}

TEST(mockFileIo, CloseCall_ReturnSuccess_ErrnoNotSet)
{
    mockFileIo_SetCloseToFail(0, EIO);
    errno = 0;
    int result = close(3);
    CHECK_EQUAL(0, result);
    CHECK_EQUAL(0, errno);
}

TEST(mockFileIo, SetUnlinkCallToFail)
{
    mockFileIo_SetUnlinkToFail(-1, EIO);
    int result = unlink("");
    CHECK_EQUAL(-1, result);
    CHECK_EQUAL(EIO, errno);
}

TEST(mockFileIo, UnlinkCall_ReturnSuccess_ErrnoNotSet)
{
    mockFileIo_SetUnlinkToFail(0, EIO);
    errno = 0;
    int result = unlink("");
    CHECK_EQUAL(0, result);
    CHECK_EQUAL(0, errno);
}

TEST(mockFileIo, SetRenameCallToFail)
{
    mockFileIo_SetRenameToFail(-1, EIO);
    int result = rename("foo", "bar");
    CHECK_EQUAL(-1, result);
    CHECK_EQUAL(EIO, errno);
}

TEST(mockFileIo, RenameCall_ReturnSuccess_ErrnoNotSet)
{
    mockFileIo_SetRenameToFail(0, EIO);
    errno = 0;
    int result = rename("foo", "bar");
    CHECK_EQUAL(0, result);
    CHECK_EQUAL(0, errno);
}
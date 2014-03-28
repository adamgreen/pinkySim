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
#include <string.h>
#include <limits.h>

extern "C"
{
#include "buffer.h"
#include "try_catch.h"
}

// Include C++ headers for test harness.
#include "CppUTest/TestHarness.h"

TEST_GROUP(Buffer)
{
    Buffer            m_buffer;
    char*             m_pCharacterArray;
    size_t            m_bufferSize;
    int               m_exceptionThrown;
    int               m_validateBufferLimits;
    static const char m_fillChar = 0xFF;
    
    void setup()
    {
        m_exceptionThrown = 0;
        m_validateBufferLimits = 1;
        memset(&m_buffer, 0, sizeof(m_buffer));
    }

    void teardown()
    {
        validateBufferLimits();
        free(m_pCharacterArray);
    }
    
    void allocateBuffer(size_t sizeOfBuffer)
    {
        m_pCharacterArray = (char*)malloc(sizeOfBuffer);
        memset(m_pCharacterArray, m_fillChar, sizeOfBuffer);
        m_bufferSize = sizeOfBuffer;
        bufferInit(&m_buffer, m_pCharacterArray, m_bufferSize);
    }
    
    void allocateBuffer(const char* pString)
    {
        allocateBuffer(strlen(pString));
        memcpy(m_pCharacterArray, pString, strlen(pString));
    }
    
    void validateBufferLimits()
    {
        if (!m_validateBufferLimits)
            return;
            
        POINTERS_EQUAL(m_pCharacterArray, m_buffer.pStart);
        POINTERS_EQUAL(m_pCharacterArray + m_bufferSize, m_buffer.pEnd);
    }
    
    void validateResetBuffer()
    {
        POINTERS_EQUAL(m_pCharacterArray, m_buffer.pCurrent);
    }
    
    void validateDepletedBufferNoOverrun()
    {
        validateNoException();
        CHECK_FALSE( bufferOverrunDetected(&m_buffer) );
        LONGS_EQUAL( 0, bufferBytesLeft(&m_buffer) );
    }
    
    void validateDepletedBufferWithOverrun()
    {
        validateBufferOverrunException();
        CHECK_TRUE( bufferOverrunDetected(&m_buffer) );
        LONGS_EQUAL( 0, bufferBytesLeft(&m_buffer) );
    }
    
    void validateInvalidHexDigitException()
    {
        CHECK_TRUE( m_exceptionThrown );
        LONGS_EQUAL( invalidHexDigitException, getExceptionCode() );
    }
    
    void validateInvalidValueException()
    {
        CHECK_TRUE( m_exceptionThrown );
        LONGS_EQUAL( invalidValueException, getExceptionCode() );
    }
    
    void validateBufferOverrunException()
    {
        CHECK_TRUE( m_exceptionThrown );
        LONGS_EQUAL( bufferOverrunException, getExceptionCode() );
    }
    
    void validateNoException()
    {
        CHECK_FALSE( m_exceptionThrown );
        LONGS_EQUAL( noException, getExceptionCode() );
    }
    
    void writeSpaces(size_t numberToWrite)
    {
        for (size_t i = 0 ; i < numberToWrite ; i++)
            bufferWriteChar(&m_buffer, ' ');
    }
};

TEST(Buffer, bufferInit)
{
    allocateBuffer(1);
    validateResetBuffer();
    BYTES_EQUAL(m_fillChar, m_pCharacterArray[0]);
}

TEST(Buffer, bufferReset)
{
    allocateBuffer(1);
    m_buffer.pCurrent = NULL;
    bufferReset(&m_buffer);
    validateResetBuffer();
    BYTES_EQUAL(m_fillChar, m_pCharacterArray[0]);
}

TEST(Buffer, bufferGetLength_0)
{
    allocateBuffer((size_t)0);
    
    LONGS_EQUAL( 0, bufferGetLength(&m_buffer) );
}

TEST(Buffer, bufferGetLength_WithInvalidPointers)
{
    m_buffer.pStart = (char*)~0;
    m_buffer.pEnd = NULL;
    LONGS_EQUAL( 0, bufferGetLength(&m_buffer) );
    allocateBuffer(1);
}

TEST(Buffer, bufferGetLength_512)
{
    allocateBuffer(512);
    
    LONGS_EQUAL( 512, bufferGetLength(&m_buffer) );
}

TEST(Buffer, bufferSetEndOfBuffer)
{
    allocateBuffer(512);
    
    writeSpaces(128);
    bufferSetEndOfBuffer(&m_buffer);
    LONGS_EQUAL( 128, bufferGetLength(&m_buffer) );
    LONGS_EQUAL( 0, bufferBytesLeft(&m_buffer) );
    m_validateBufferLimits = 0;
}

TEST(Buffer, bufferGetArray)
{
    allocateBuffer(1);
    
    POINTERS_EQUAL( m_pCharacterArray, bufferGetArray(&m_buffer) );
}

TEST(Buffer, bufferBytesLeft_Empty)
{
    allocateBuffer(1);
    size_t bytesLeft = bufferBytesLeft(&m_buffer);
    LONGS_EQUAL(1, bytesLeft);
}

TEST(Buffer, bufferByteLeft_Overrun_NoThrow)
{
    size_t bytesLeft = ~0U;
    
    allocateBuffer(1);
    __try_and_catch(writeSpaces(2));
    
    __try
        bytesLeft = bufferBytesLeft(&m_buffer);
    __catch
        FAIL( "bufferBytesLeft() threw exception." );
        
    LONGS_EQUAL(bytesLeft, 0);
}

TEST(Buffer, bufferWriteChar_Full_No_Overrun)
{
    allocateBuffer(1);

    __try
        writeSpaces(1);
    __catch
        m_exceptionThrown = 1;
        
    BYTES_EQUAL( ' ', m_pCharacterArray[0]);
    validateDepletedBufferNoOverrun();
}

TEST(Buffer, bufferWriteChar_Full_Overrun)
{
    allocateBuffer(1);

    __try
        writeSpaces(2);
    __catch
        m_exceptionThrown = 1;

    BYTES_EQUAL( ' ', m_pCharacterArray[0] );
    validateDepletedBufferWithOverrun();
}

TEST(Buffer, bufferReadChar_No_Overrun)
{
    char         characterRead = 0x00;
    
    allocateBuffer(1);

    __try
        characterRead = bufferReadChar(&m_buffer);
    __catch
        m_exceptionThrown = 1;

    BYTES_EQUAL( m_fillChar, characterRead );
    validateDepletedBufferNoOverrun();
}

TEST(Buffer, bufferReadChar_Overrun)
{
    char         characterRead = 0x00;
    
    allocateBuffer(1);
    bufferReadChar(&m_buffer);

    __try
        characterRead = bufferReadChar(&m_buffer);
    __catch
        m_exceptionThrown = 1;
    BYTES_EQUAL( '\0', characterRead );
    validateDepletedBufferWithOverrun();
}

TEST(Buffer, bufferWriteByteAsHex_Full_No_Overrun)
{
    static const unsigned char testByte = 0x5A;
    static const char          expectedString[] = "5a";
    
    allocateBuffer(2);

    __try
        bufferWriteByteAsHex(&m_buffer, testByte);
    __catch
        m_exceptionThrown = 1;
    CHECK( 0 == memcmp(m_pCharacterArray, expectedString, 2) );
    validateDepletedBufferNoOverrun();
}

TEST(Buffer, bufferWriteByteAsHex_Full_OverrunBy2Bytes)
{
    static const unsigned char testByte = 0x5A;
    static const char          expectedString[] = "5a";
    
    allocateBuffer(2);

    bufferWriteByteAsHex(&m_buffer, testByte);
    __try
        bufferWriteByteAsHex(&m_buffer, testByte);
    __catch
        m_exceptionThrown = 1;
    CHECK( 0 == memcmp(m_pCharacterArray, expectedString, 2) );
    validateDepletedBufferWithOverrun();
}

TEST(Buffer, bufferWriteByteAsHex_Full_OverrunBy1Bytes)
{
    static const unsigned char testByte = 0x5A;
    static const char          expectedString[] = "5a";
    
    allocateBuffer(3);

    bufferWriteByteAsHex(&m_buffer, testByte);
    __try
        bufferWriteByteAsHex(&m_buffer, testByte);
    __catch
        m_exceptionThrown = 1;
    CHECK( 0 == memcmp(m_pCharacterArray, expectedString, 2) );
    validateDepletedBufferWithOverrun();
}

TEST(Buffer, bufferReadByteAsHex_Lowercase)
{
    static const unsigned char testByte = 0x5A;
    static const char          testString[] = "5a";
    unsigned char              byteRead = 0;
    
    allocateBuffer(testString);

    __try
        byteRead = bufferReadByteAsHex(&m_buffer);
    __catch
        m_exceptionThrown = 1;

    BYTES_EQUAL( testByte, byteRead );
    validateDepletedBufferNoOverrun();
}

TEST(Buffer, bufferReadByteAsHex_Uppercase)
{
    static const unsigned char testByte = 0x5A;
    static const char          testString[] = "5A";
    unsigned char              byteRead = 0;
    
    allocateBuffer(testString);

    __try
        byteRead = bufferReadByteAsHex(&m_buffer);
    __catch
        m_exceptionThrown = 1;
    BYTES_EQUAL( testByte, byteRead );
    validateDepletedBufferNoOverrun();
}

TEST(Buffer, bufferReadByteAsHex_InvalidFirstHexDigit)
{
    unsigned char       byteRead = 0;
    static const char   testString[] = "\xFF\xFF";
    
    allocateBuffer(testString);
    __try
        byteRead = bufferReadByteAsHex(&m_buffer);
    __catch
        m_exceptionThrown = 1;
    BYTES_EQUAL( 0x00, byteRead );
    LONGS_EQUAL( 2, bufferBytesLeft(&m_buffer) );
    validateInvalidHexDigitException();
}

TEST(Buffer, bufferReadByteAsHex_InvalidSecondHexDigit)
{
    unsigned char       byteRead = 0;
    static const char   testString[] = "f\xFF";
    
    allocateBuffer(testString);
    
    __try
        byteRead = bufferReadByteAsHex(&m_buffer);
    __catch
        m_exceptionThrown = 1;
    BYTES_EQUAL( 0x00, byteRead );
    LONGS_EQUAL( 2, bufferBytesLeft(&m_buffer) );
    validateInvalidHexDigitException();
}

TEST(Buffer, bufferReadByteAsHex_Full_Overrun)
{
    static const char          testString[] = "5a";
    unsigned char              byteRead = 0;
    
    allocateBuffer(testString);

    bufferReadByteAsHex(&m_buffer);
    __try
        byteRead = bufferReadByteAsHex(&m_buffer);
    __catch
        m_exceptionThrown = 1;
    BYTES_EQUAL( 0x00, byteRead );
    validateDepletedBufferWithOverrun();
}

TEST(Buffer, bufferReadByteAsHex_Full_OverrunBy1)
{
    static const char          testString[] = "5af";
    unsigned char              byteRead = 0;
    
    allocateBuffer(testString);

    bufferReadByteAsHex(&m_buffer);
    __try
        byteRead = bufferReadByteAsHex(&m_buffer);
    __catch
        m_exceptionThrown = 1;
    BYTES_EQUAL( 0x00, byteRead );
    validateDepletedBufferWithOverrun();
}

TEST(Buffer, bufferWriteString_Full_No_Overrun)
{
    static const char   testString[] = "Hi";
    
    allocateBuffer(2);
    __try
        bufferWriteString(&m_buffer, testString);
    __catch
        m_exceptionThrown = 1;

    CHECK( 0 == memcmp(m_pCharacterArray, testString, 2) );
    validateDepletedBufferNoOverrun();
}

TEST(Buffer, bufferWriteString_Full_Overrun)
{
    static const char   testString[] = "Hi";

    allocateBuffer(1);
    __try
        bufferWriteString(&m_buffer, testString);
    __catch
        m_exceptionThrown = 1;
    BYTES_EQUAL( m_fillChar, m_pCharacterArray[0] );
    validateDepletedBufferWithOverrun();
}

TEST(Buffer, bufferWriteSizedString_Full_No_Overrun)
{
    static const char   testString[] = "Hi";
    
    allocateBuffer(2);
    __try
        bufferWriteSizedString(&m_buffer, testString, 2);
    __catch
        m_exceptionThrown = 1;

    CHECK( 0 == memcmp(m_pCharacterArray, testString, 2) );
    validateDepletedBufferNoOverrun();
}

TEST(Buffer, bufferWriteSizedString_Full_Overrun)
{
    static const char   testString[] = "Hi";

    allocateBuffer(1);
    __try
        bufferWriteSizedString(&m_buffer, testString, 2);
    __catch
        m_exceptionThrown = 1;
    BYTES_EQUAL( m_fillChar, m_pCharacterArray[0] );
    validateDepletedBufferWithOverrun();
}

TEST(Buffer, bufferReadUIntegerAsHex_JustComma)
{
    static const char testString[] = ",";
    uint32_t          value = ~0U;
    
    allocateBuffer(testString);
    __try
        value = bufferReadUIntegerAsHex(&m_buffer);
    __catch
        m_exceptionThrown = 1;
    LONGS_EQUAL( 1, bufferBytesLeft(&m_buffer) );
    validateInvalidValueException();
}

TEST(Buffer, bufferReadUIntegerAsHex_Empty)
{
    static const char testString[] = "";
    uint32_t          value = ~0U;
    
    allocateBuffer(testString);
    __try
        value = bufferReadUIntegerAsHex(&m_buffer);
    __catch
        m_exceptionThrown = 1;
    LONGS_EQUAL( 0, bufferBytesLeft(&m_buffer) );
    validateInvalidValueException();
}

TEST(Buffer, bufferReadUIntegerAsHex_0)
{
    static const char testString[] = "0";
    uint32_t          value = ~0U;
    
    allocateBuffer(testString);
    __try
        value = bufferReadUIntegerAsHex(&m_buffer);
    __catch
        m_exceptionThrown = 1;
    LONGS_EQUAL( 0, value );
    validateDepletedBufferNoOverrun();
}

TEST(Buffer, bufferReadUIntegerAsHex_f)
{
    static const char testString[] = "f";
    uint32_t          value = ~0U;
    
    allocateBuffer(testString);
    __try
        value = bufferReadUIntegerAsHex(&m_buffer);
    __catch
        m_exceptionThrown = 1;
    LONGS_EQUAL( 0xf, value );
    validateDepletedBufferNoOverrun();
}

TEST(Buffer, bufferReadUIntegerAsHex_F)
{
    static const char testString[] = "F";
    uint32_t          value = ~0U;
    
    allocateBuffer(testString);
    __try
        value = bufferReadUIntegerAsHex(&m_buffer);
    __catch
        m_exceptionThrown = 1;
    LONGS_EQUAL( 0xf, value );
    validateDepletedBufferNoOverrun();
}

TEST(Buffer, bufferReadUIntegerAsHex_00000000)
{
    static const char testString[] = "00000000";
    uint32_t          value = ~0U;
    
    allocateBuffer(testString);
    __try
        value = bufferReadUIntegerAsHex(&m_buffer);
    __catch
        m_exceptionThrown = 1;
    LONGS_EQUAL( 0x0, value );
    validateDepletedBufferNoOverrun();
}

TEST(Buffer, bufferReadUIntegerAsHex_ffffffff)
{
    static const char testString[] = "ffffffff";
    uint32_t          value = 0;
    
    allocateBuffer(testString);
    __try
        value = bufferReadUIntegerAsHex(&m_buffer);
    __catch
        m_exceptionThrown = 1;
    LONGS_EQUAL( 0xffffffff, value );
    validateDepletedBufferNoOverrun();
}

TEST(Buffer, bufferReadUIntegerAsHex_12345678comma)
{
    static const char testString[] = "12345678,";
    uint32_t          value = ~0U;
    
    allocateBuffer(testString);
    __try
        value = bufferReadUIntegerAsHex(&m_buffer);
    __catch
        m_exceptionThrown = 1;
    LONGS_EQUAL( 0x12345678, value );
    LONGS_EQUAL( 1, bufferBytesLeft(&m_buffer) );
    validateNoException();
}

TEST(Buffer, bufferWriteUIntegerAsHex_0)
{
    static const char expectedString[] = "00";
    
    allocateBuffer(strlen(expectedString));
    __try
        bufferWriteUIntegerAsHex(&m_buffer, 0);
    __catch
        m_exceptionThrown = 1;
    CHECK( 0 == memcmp(m_pCharacterArray, expectedString, strlen(expectedString)) );
    LONGS_EQUAL( 0, bufferBytesLeft(&m_buffer) );
    validateNoException();
}

TEST(Buffer, bufferWriteUIntegerAsHex_Full_Overrun)
{
    allocateBuffer(1);
    __try
        bufferWriteUIntegerAsHex(&m_buffer, 0);
    __catch
        m_exceptionThrown = 1;
    validateDepletedBufferWithOverrun();
}

TEST(Buffer, bufferWriteUIntegerAsHex_0f)
{
    static const char expectedString[] = "0f";
    
    allocateBuffer(strlen(expectedString));
    __try
        bufferWriteUIntegerAsHex(&m_buffer, 0x0F);
    __catch
        m_exceptionThrown = 1;
    CHECK( 0 == memcmp(m_pCharacterArray, expectedString, strlen(expectedString)) );
    LONGS_EQUAL( 0, bufferBytesLeft(&m_buffer) );
    validateNoException();
}

TEST(Buffer, bufferWriteUIntegerAsHex_0fed_Overrun)
{
    static const char expectedString[] = "0fed";
    
    allocateBuffer(2);
    __try
        bufferWriteUIntegerAsHex(&m_buffer, 0x0FED);
    __catch
        m_exceptionThrown = 1;
    CHECK( 0 == memcmp(m_pCharacterArray, expectedString, 2) );
    validateDepletedBufferWithOverrun();
}

TEST(Buffer, bufferWriteUIntegerAsHex_12)
{
    static const char expectedString[] = "12";
    
    allocateBuffer(strlen(expectedString));
    __try
        bufferWriteUIntegerAsHex(&m_buffer, 0x12);
    __catch
        m_exceptionThrown = 1;
    CHECK( 0 == memcmp(m_pCharacterArray, expectedString, strlen(expectedString)) );
    LONGS_EQUAL( 0, bufferBytesLeft(&m_buffer) );
    validateNoException();
}

TEST(Buffer, bufferWriteUIntegerAsHex_1234)
{
    static const char expectedString[] = "1234";
    
    allocateBuffer(strlen(expectedString));
    __try
        bufferWriteUIntegerAsHex(&m_buffer, 0x1234);
    __catch
        m_exceptionThrown = 1;
    CHECK( 0 == memcmp(m_pCharacterArray, expectedString, strlen(expectedString)) );
    LONGS_EQUAL( 0, bufferBytesLeft(&m_buffer) );
    validateNoException();
}

TEST(Buffer, bufferWriteUIntegerAsHex_123456)
{
    static const char expectedString[] = "123456";
    
    allocateBuffer(strlen(expectedString));
    __try
        bufferWriteUIntegerAsHex(&m_buffer, 0x123456);
    __catch
        m_exceptionThrown = 1;
    CHECK( 0 == memcmp(m_pCharacterArray, expectedString, strlen(expectedString)) );
    LONGS_EQUAL( 0, bufferBytesLeft(&m_buffer) );
    validateNoException();
}

TEST(Buffer, bufferWriteUIntegerAsHex_01234567)
{
    static const char expectedString[] = "01234567";
    
    allocateBuffer(strlen(expectedString));
    __try
        bufferWriteUIntegerAsHex(&m_buffer, 0x01234567);
    __catch
        m_exceptionThrown = 1;
    CHECK( 0 == memcmp(m_pCharacterArray, expectedString, strlen(expectedString)) );
    LONGS_EQUAL( 0, bufferBytesLeft(&m_buffer) );
    validateNoException();
}

TEST(Buffer, bufferWriteUIntegerAsHex_12345678)
{
    static const char expectedString[] = "12345678";
    
    allocateBuffer(strlen(expectedString));
    __try
        bufferWriteUIntegerAsHex(&m_buffer, 0x12345678);
    __catch
        m_exceptionThrown = 1;
    CHECK( 0 == memcmp(m_pCharacterArray, expectedString, strlen(expectedString)) );
    LONGS_EQUAL( 0, bufferBytesLeft(&m_buffer) );
    validateNoException();
}

TEST(Buffer, bufferReadIntegerAsHex_0)
{
    static const char testString[] = "0";
    int32_t           value = -1;
    
    allocateBuffer(testString);
    __try
        value = bufferReadIntegerAsHex(&m_buffer);
    __catch
        m_exceptionThrown = 1;
    LONGS_EQUAL( 0, value );
    validateNoException();
}

TEST(Buffer, bufferReadIntegerAsHex_Neg0)
{
    static const char testString[] = "-0";
    int32_t           value = -1;
    
    allocateBuffer(testString);
    __try
        value = bufferReadIntegerAsHex(&m_buffer);
    __catch
        m_exceptionThrown = 1;
    LONGS_EQUAL( 0, value );
    validateNoException();
}

TEST(Buffer, bufferReadIntegerAsHex_00)
{
    static const char testString[] = "00";
    int32_t           value = -1;
    
    allocateBuffer(testString);
    __try
        value = bufferReadIntegerAsHex(&m_buffer);
    __catch
        m_exceptionThrown = 1;
    LONGS_EQUAL( 0, value );
    validateNoException();
}

TEST(Buffer, bufferReadIntegerAsHex_1)
{
    static const char testString[] = "1";
    int32_t           value = -1;
    
    allocateBuffer(testString);
    __try
        value = bufferReadIntegerAsHex(&m_buffer);
    __catch
        m_exceptionThrown = 1;
    LONGS_EQUAL( 1, value );
    validateNoException();
}

TEST(Buffer, bufferReadIntegerAsHex_Neg1)
{
    static const char testString[] = "-1";
    int32_t           value = 0;
    
    allocateBuffer(testString);
    __try
        value = bufferReadIntegerAsHex(&m_buffer);
    __catch
        m_exceptionThrown = 1;
    LONGS_EQUAL( -1, value );
    validateNoException();
}

TEST(Buffer, bufferReadIntegerAsHex_Empty)
{
    int32_t           value = -1;
    
    allocateBuffer((size_t)0);
    __try
        value = bufferReadIntegerAsHex(&m_buffer);
    __catch
        m_exceptionThrown = 1;
    validateBufferOverrunException();
}

TEST(Buffer, bufferReadIntegerAsHex_NegComma)
{
    static const char testString[] = "-,";
    int32_t           value = -1;
    
    allocateBuffer(testString);
    __try
        value = bufferReadIntegerAsHex(&m_buffer);
    __catch
        m_exceptionThrown = 1;
    validateInvalidValueException();
}

TEST(Buffer, bufferReadIntegerAsHex_Neg)
{
    static const char testString[] = "-";
    int32_t           value = -1;
    
    allocateBuffer(testString);
    __try
        value = bufferReadIntegerAsHex(&m_buffer);
    __catch
        m_exceptionThrown = 1;
    validateInvalidValueException();
}

TEST(Buffer, bufferReadIntegerAsHex_7fffffff)
{
    static const char testString[] = "7fffffff";
    int32_t           value = -1;
    
    allocateBuffer(testString);
    __try
        value = bufferReadIntegerAsHex(&m_buffer);
    __catch
        m_exceptionThrown = 1;
    LONGS_EQUAL( 0x7FFFFFFF, value );
    validateNoException();
}

TEST(Buffer, bufferReadIntegerAsHex_Neg80000000)
{
    static const char testString[] = "-80000000";
    int32_t           value = -1;
    
    allocateBuffer(testString);
    __try
        value = bufferReadIntegerAsHex(&m_buffer);
    __catch
        m_exceptionThrown = 1;
    LONGS_EQUAL( INT_MIN, value );
    validateNoException();
}

TEST(Buffer, bufferReadIntegerAsHex_Neg80000001)
{
    static const char testString[] = "-80000001";
    int32_t           value = -1;
    
    allocateBuffer(testString);
    __try
        value = bufferReadIntegerAsHex(&m_buffer);
    __catch
        m_exceptionThrown = 1;
    validateInvalidValueException();
}

TEST(Buffer, bufferReadIntegerAsHex_80000000)
{
    static const char testString[] = "80000000";
    int32_t           value = -1;
    
    allocateBuffer(testString);
    __try
        value = bufferReadIntegerAsHex(&m_buffer);
    __catch
        m_exceptionThrown = 1;
    validateInvalidValueException();
}

TEST(Buffer, bufferWriteIntegerAsHex_00)
{
    static const char       expectedString[] = "00";
    static const int32_t    testValue = 0;
    
    allocateBuffer(strlen(expectedString));
    __try
        bufferWriteIntegerAsHex(&m_buffer, testValue);
    __catch
        m_exceptionThrown = 1;
    CHECK( 0 == memcmp(m_pCharacterArray, expectedString, strlen(expectedString)) );
    validateNoException();
}

TEST(Buffer, bufferWriteIntegerAsHex_1)
{
    static const char       expectedString[] = "01";
    static const int32_t    testValue = 1;
    
    allocateBuffer(strlen(expectedString));
    __try
        bufferWriteIntegerAsHex(&m_buffer, testValue);
    __catch
        m_exceptionThrown = 1;
    CHECK( 0 == memcmp(m_pCharacterArray, expectedString, strlen(expectedString)) );
    validateNoException();
}

TEST(Buffer, bufferWriteIntegerAsHex_Neg1)
{
    static const char       expectedString[] = "-01";
    static const int32_t    testValue = -1;
    
    allocateBuffer(strlen(expectedString));
    __try
        bufferWriteIntegerAsHex(&m_buffer, testValue);
    __catch
        m_exceptionThrown = 1;
    CHECK( 0 == memcmp(m_pCharacterArray, expectedString, strlen(expectedString)) );
    validateNoException();
}

TEST(Buffer, bufferWriteIntegerAsHex_MAXINT)
{
    static const char       expectedString[] = "7fffffff";
    static const int32_t    testValue = INT_MAX;
    
    allocateBuffer(strlen(expectedString));
    __try
        bufferWriteIntegerAsHex(&m_buffer, testValue);
    __catch
        m_exceptionThrown = 1;
    CHECK( 0 == memcmp(m_pCharacterArray, expectedString, strlen(expectedString)) );
    validateNoException();
}

TEST(Buffer, bufferWriteIntegerAsHex_MININT)
{
    static const char       expectedString[] = "-80000000";
    static const int32_t    testValue = INT_MIN;
    
    allocateBuffer(strlen(expectedString));
    __try
        bufferWriteIntegerAsHex(&m_buffer, testValue);
    __catch
        m_exceptionThrown = 1;
    CHECK( 0 == memcmp(m_pCharacterArray, expectedString, strlen(expectedString)) );
    validateNoException();
}

TEST(Buffer, bufferWriteIntegerAsHex_Empty)
{
    static const int32_t    testValue = -1;
    
    allocateBuffer((size_t)0);
    __try
        bufferWriteIntegerAsHex(&m_buffer, testValue);
    __catch
        m_exceptionThrown = 1;
    validateDepletedBufferWithOverrun();
}

TEST(Buffer, bufferWriteIntegerAsHex_RoomForMinusOnly)
{
    static const char       expectedString[] = "-";
    static const int32_t    testValue = -1;
    
    allocateBuffer(strlen(expectedString));
    __try
        bufferWriteIntegerAsHex(&m_buffer, testValue);
    __catch
        m_exceptionThrown = 1;
    CHECK( 0 == memcmp(m_pCharacterArray, expectedString, strlen(expectedString)) );
    validateDepletedBufferWithOverrun();
}

TEST(Buffer, bufferIsNextCharEqualTo_Empty)
{
    static const char   testString[] = "";
    static const char   testChar = ':';
    int                 isEqual = -1;
    
    allocateBuffer(testString);
    __try
        isEqual = bufferIsNextCharEqualTo(&m_buffer, testChar);
    __catch
        m_exceptionThrown = 1;
    validateBufferOverrunException();
}

TEST(Buffer, bufferIsNextCharEqualTo_Match)
{
    static const char   testString[] = ":";
    static const char   testChar = ':';
    int                 isEqual = -1;
    
    allocateBuffer(testString);
    __try
        isEqual = bufferIsNextCharEqualTo(&m_buffer, testChar);
    __catch
        m_exceptionThrown = 1;
    CHECK_TRUE( isEqual );
    LONGS_EQUAL( 0, bufferBytesLeft(&m_buffer) );
    validateNoException();
}

TEST(Buffer, bufferIsNextCharEqualTo_NoMatch)
{
    static const char   testString[] = ",";
    static const char   testChar = ':';
    int                 isEqual = -1;
    
    allocateBuffer(testString);
    __try
        isEqual = bufferIsNextCharEqualTo(&m_buffer, testChar);
    __catch
        m_exceptionThrown = 1;
    CHECK_FALSE( isEqual );
    LONGS_EQUAL( 1, bufferBytesLeft(&m_buffer) );
    validateNoException();
}

TEST(Buffer, bufferMatchesString_TooSmall)
{
    static const char   testString[] = "String";
    static const char   compareString[] = "StringMatch";
    int                 isEqual = -1;
    
    allocateBuffer(testString);
    __try
        isEqual = bufferMatchesString(&m_buffer, compareString, sizeof(compareString)-1);
    __catch
        m_exceptionThrown = 1;
    validateBufferOverrunException();
}

TEST(Buffer, bufferMatchesString_Match)
{
    static const char   testString[] = "StringMatch";
    static const char   compareString[] = "StringMatch";
    int                 isEqual = -1;
    
    allocateBuffer(testString);
    __try
        isEqual = bufferMatchesString(&m_buffer, compareString, sizeof(compareString)-1);
    __catch
        m_exceptionThrown = 1;
    CHECK_TRUE( isEqual );
    LONGS_EQUAL( 0, bufferBytesLeft(&m_buffer) );
    validateNoException();
}

TEST(Buffer, bufferMatchesString_NoMatch)
{
    static const char   testString[] = "StringMatch";
    static const char   compareString[] = "StringMatc?";
    int                 isEqual = -1;
    
    allocateBuffer(testString);
    __try
        isEqual = bufferMatchesString(&m_buffer, compareString, sizeof(compareString)-1);
    __catch
        m_exceptionThrown = 1;
    CHECK_FALSE( isEqual );
    LONGS_EQUAL( strlen(testString), bufferBytesLeft(&m_buffer) );
    validateNoException();
}

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
    #include <gdbRemote.h>
}

// Include C++ headers for test harness.
#include <CppUTest/TestHarness.h>
#include <commMock.h>

TEST_GROUP(gdbRemote)
{
    PinkySimContext m_context;
    char            m_checksummed[512];
    
    void setup()
    {
        memset(&m_context, 0x5A, sizeof(m_context));
        mockCommInitTransmitDataBuffer(512);
    }

    void teardown()
    {
        mockCommUninitTransmitDataBuffer();
    }
    
    char* appendChecksums(const char* pOrig)
    {
        const char* pSrc = pOrig;
        char*       pDest = m_checksummed;
        uint8_t     checksum;
        
        while (*pSrc)
        {
            // Copy through to start of packet byte.
            while (*pSrc && *pSrc != '$')
                *pDest++ = *pSrc++;
            *pDest++ = *pSrc++;
            if (pSrc[-1] == '\0')
                return m_checksummed;
        
            // Copy and checksum data up to end of packet byte.
            checksum = 0;
            while (*pSrc && *pSrc != '#')
            {
                checksum += *pSrc;
                *pDest++ = *pSrc++;
            }
            *pDest++ = *pSrc++;
            if (pSrc[-1] == '\0')
                return m_checksummed;
        
            // Append the checksum value.
            sprintf(pDest, "%02x", checksum);
            pDest += 2;
        }        
        *pDest++ = *pSrc++;
        return m_checksummed;
    }
    
    void fillContextRegistersWithAscendingValues()
    {
        for (int i = 0 ; i < sizeof(m_context.R)/sizeof(m_context.R[0]) ; i++)
            m_context.R[i] = 0x11111111 * i;
        m_context.spMain = 0xDDDDDDDD;
        m_context.lr = 0xEEEEEEEE;
        m_context.pc = 0xFFFFFFFF;
    }
};



TEST(gdbRemote, gdbRemoteGetRegisters_SendsGetRegistersCommandAndParsesResponseCorrectly)
{
    mockCommInitReceiveData(appendChecksums(
                            "+$"
                            "00000000"
                            "11111111"
                            "22222222"
                            "33333333"
                            "44444444"
                            "55555555"
                            "66666666"
                            "77777777"
                            "88888888"
                            "99999999"
                            "AAAAAAAA"
                            "BBBBBBBB"
                            "CCCCCCCC"
                            "DDDDDDDD"
                            "EEEEEEEE"
                            "FFFFFFFF"
                            "78563412"
                            "#"));

    gdbRemoteGetRegisters(&m_context);

    CHECK( mockCommDoesTransmittedDataEqual(appendChecksums("$g#+")) );
    CHECK_EQUAL(0x00000000, m_context.R[R0]);
    CHECK_EQUAL(0x11111111, m_context.R[R1]);
    CHECK_EQUAL(0x22222222, m_context.R[R2]);
    CHECK_EQUAL(0x33333333, m_context.R[R3]);
    CHECK_EQUAL(0x44444444, m_context.R[R4]);
    CHECK_EQUAL(0x55555555, m_context.R[R5]);
    CHECK_EQUAL(0x66666666, m_context.R[R6]);
    CHECK_EQUAL(0x77777777, m_context.R[R7]);
    CHECK_EQUAL(0x88888888, m_context.R[R8]);
    CHECK_EQUAL(0x99999999, m_context.R[R9]);
    CHECK_EQUAL(0xAAAAAAAA, m_context.R[R10]);
    CHECK_EQUAL(0xBBBBBBBB, m_context.R[R11]);
    CHECK_EQUAL(0xCCCCCCCC, m_context.R[R12]);
    CHECK_EQUAL(0xDDDDDDDD, m_context.spMain);
    CHECK_EQUAL(0xEEEEEEEE, m_context.lr);
    CHECK_EQUAL(0xFFFFFFFF, m_context.pc);
    CHECK_EQUAL(0x12345678, m_context.xPSR);
}

TEST(gdbRemote, gdbRemoteGetRegisters_TruncatedResponseDataShouldResultInThrownException)
{
    mockCommInitReceiveData(appendChecksums(
                            "+$"
                            "00000000"
                            "11111111"
                            "22222222"
                            "33333333"
                            "44444444"
                            "55555555"
                            "66666666"
                            "77777777"
                            "88888888"
                            "99999999"
                            "AAAAAAAA"
                            "BBBBBBBB"
                            "CCCCCCCC"
                            "DDDDDDDD"
                            "EEEEEEEE"
                            "FFFFFFFF"
                            "#"));

    __try_and_catch( gdbRemoteGetRegisters(&m_context) );

    CHECK_EQUAL(bufferOverrunException, getExceptionCode());
    CHECK( mockCommDoesTransmittedDataEqual(appendChecksums("$g#+")) );
}

TEST(gdbRemote, gdbRemoteGetRegisters_HandleBadChecksumInResponseData)
{
    mockCommInitReceiveData("+$"
                            "00000000"
                            "11111111"
                            "22222222"
                            "33333333"
                            "44444444"
                            "55555555"
                            "66666666"
                            "77777777"
                            "88888888"
                            "99999999"
                            "AAAAAAAA"
                            "BBBBBBBB"
                            "CCCCCCCC"
                            "DDDDDDDD"
                            "EEEEEEEE"
                            "FFFFFFFF"
                            "78563412"
                            "#00",
                            "$"
                            "00000000"
                            "11111111"
                            "22222222"
                            "33333333"
                            "44444444"
                            "55555555"
                            "66666666"
                            "77777777"
                            "88888888"
                            "99999999"
                            "AAAAAAAA"
                            "BBBBBBBB"
                            "CCCCCCCC"
                            "DDDDDDDD"
                            "EEEEEEEE"
                            "FFFFFFFF"
                            "78563412"
                            "#B4");

    gdbRemoteGetRegisters(&m_context);

    CHECK( mockCommDoesTransmittedDataEqual(appendChecksums("$g#-+")) );
    CHECK_EQUAL(0x00000000, m_context.R[R0]);
    CHECK_EQUAL(0x11111111, m_context.R[R1]);
    CHECK_EQUAL(0x22222222, m_context.R[R2]);
    CHECK_EQUAL(0x33333333, m_context.R[R3]);
    CHECK_EQUAL(0x44444444, m_context.R[R4]);
    CHECK_EQUAL(0x55555555, m_context.R[R5]);
    CHECK_EQUAL(0x66666666, m_context.R[R6]);
    CHECK_EQUAL(0x77777777, m_context.R[R7]);
    CHECK_EQUAL(0x88888888, m_context.R[R8]);
    CHECK_EQUAL(0x99999999, m_context.R[R9]);
    CHECK_EQUAL(0xAAAAAAAA, m_context.R[R10]);
    CHECK_EQUAL(0xBBBBBBBB, m_context.R[R11]);
    CHECK_EQUAL(0xCCCCCCCC, m_context.R[R12]);
    CHECK_EQUAL(0xDDDDDDDD, m_context.spMain);
    CHECK_EQUAL(0xEEEEEEEE, m_context.lr);
    CHECK_EQUAL(0xFFFFFFFF, m_context.pc);
    CHECK_EQUAL(0x12345678, m_context.xPSR);
}

TEST(gdbRemote, gdbRemoteGetRegisters_NakCommandOnceToMakeSureThatItGetsResent)
{
    mockCommInitReceiveData(appendChecksums(
                            "-+$"
                            "00000000"
                            "11111111"
                            "22222222"
                            "33333333"
                            "44444444"
                            "55555555"
                            "66666666"
                            "77777777"
                            "88888888"
                            "99999999"
                            "AAAAAAAA"
                            "BBBBBBBB"
                            "CCCCCCCC"
                            "DDDDDDDD"
                            "EEEEEEEE"
                            "FFFFFFFF"
                            "78563412"
                            "#"));

    gdbRemoteGetRegisters(&m_context);

    CHECK( mockCommDoesTransmittedDataEqual(appendChecksums("$g#$g#+")) );
    CHECK_EQUAL(0x00000000, m_context.R[R0]);
    CHECK_EQUAL(0x11111111, m_context.R[R1]);
    CHECK_EQUAL(0x22222222, m_context.R[R2]);
    CHECK_EQUAL(0x33333333, m_context.R[R3]);
    CHECK_EQUAL(0x44444444, m_context.R[R4]);
    CHECK_EQUAL(0x55555555, m_context.R[R5]);
    CHECK_EQUAL(0x66666666, m_context.R[R6]);
    CHECK_EQUAL(0x77777777, m_context.R[R7]);
    CHECK_EQUAL(0x88888888, m_context.R[R8]);
    CHECK_EQUAL(0x99999999, m_context.R[R9]);
    CHECK_EQUAL(0xAAAAAAAA, m_context.R[R10]);
    CHECK_EQUAL(0xBBBBBBBB, m_context.R[R11]);
    CHECK_EQUAL(0xCCCCCCCC, m_context.R[R12]);
    CHECK_EQUAL(0xDDDDDDDD, m_context.spMain);
    CHECK_EQUAL(0xEEEEEEEE, m_context.lr);
    CHECK_EQUAL(0xFFFFFFFF, m_context.pc);
    CHECK_EQUAL(0x12345678, m_context.xPSR);
}



TEST(gdbRemote, gdbRemoteSetRegisters_SendsSetRegistersCommandAndReceivesExpectedOkResult)
{
    mockCommInitReceiveData(appendChecksums("+$OK#"));
    fillContextRegistersWithAscendingValues();
    m_context.xPSR = 0x12345678;
    
    gdbRemoteSetRegisters(&m_context);

    CHECK( mockCommDoesTransmittedDataEqual(appendChecksums("$G"
                                                            "00000000"
                                                            "11111111"
                                                            "22222222"
                                                            "33333333"
                                                            "44444444"
                                                            "55555555"
                                                            "66666666"
                                                            "77777777"
                                                            "88888888"
                                                            "99999999"
                                                            "aaaaaaaa"
                                                            "bbbbbbbb"
                                                            "cccccccc"
                                                            "dddddddd"
                                                            "eeeeeeee"
                                                            "ffffffff"
                                                            "78563412#+")) );
}

TEST(gdbRemote, gdbRemoteSetRegisters_ThrowsExceptionIfResponseIsNotOk)
{
    mockCommInitReceiveData(appendChecksums("+$E01#"));
    fillContextRegistersWithAscendingValues();
    m_context.xPSR = 0x12345678;
    
    __try_and_catch( gdbRemoteSetRegisters(&m_context) );

    CHECK_EQUAL(badResponseException, getExceptionCode());
    CHECK( mockCommDoesTransmittedDataEqual(appendChecksums("$G"
                                                            "00000000"
                                                            "11111111"
                                                            "22222222"
                                                            "33333333"
                                                            "44444444"
                                                            "55555555"
                                                            "66666666"
                                                            "77777777"
                                                            "88888888"
                                                            "99999999"
                                                            "aaaaaaaa"
                                                            "bbbbbbbb"
                                                            "cccccccc"
                                                            "dddddddd"
                                                            "eeeeeeee"
                                                            "ffffffff"
                                                            "78563412#+")) );
}



TEST(gdbRemote, gdbRemoteReadMemory_SendsReadMemoryCommandAndParseOneByteResponseCorrectly)
{
    uint8_t buffer[1] = { 0xff };
    mockCommInitReceiveData(appendChecksums("+$a5#"));
    
    gdbRemoteReadMemory(0x10000000, buffer, 1);

    CHECK( mockCommDoesTransmittedDataEqual(appendChecksums("$m10000000,01#+")) );
    CHECK_EQUAL(0xA5, buffer[0]);
}

TEST(gdbRemote, gdbRemoteReadMemory_SendsReadMemoryCommandAndParseFourByteResponseCorrectly)
{
    uint32_t buffer = 0xFFFFFFFF;
    mockCommInitReceiveData(appendChecksums("+$78563412#"));
    
    gdbRemoteReadMemory(0x10000004, &buffer, sizeof(buffer));

    CHECK( mockCommDoesTransmittedDataEqual(appendChecksums("$m10000004,04#+")) );
    CHECK_EQUAL(0x12345678, buffer);
}

TEST(gdbRemote, gdbRemoteReadMemory_ThrowsExceptionIfResponseTooShort)
{
    uint32_t buffer = 0xFFFFFFFF;
    mockCommInitReceiveData(appendChecksums("+$785634#"));
    
    __try_and_catch( gdbRemoteReadMemory(0x10000004, &buffer, sizeof(buffer)) );
    
    CHECK_EQUAL(bufferOverrunException, getExceptionCode());
}



TEST(gdbRemote, gdbRemoteWriteMemory_SendsWriteCommandAndReceivesExpectedOkResult)
{
    uint8_t data = 0xa5;
    
    mockCommInitReceiveData(appendChecksums("+$OK#"));
    
    gdbRemoteWriteMemory(0x10000000, &data, sizeof(data));

    CHECK( mockCommDoesTransmittedDataEqual(appendChecksums("$M10000000,01:a5#+")) );
}

TEST(gdbRemote, gdbRemoteWriteMemory_Sends4ByteWriteCommandAndReceivesExpectedOkResult)
{
    uint32_t data = 0x12345678;
    
    mockCommInitReceiveData(appendChecksums("+$OK#"));
    
    gdbRemoteWriteMemory(0x10000004, &data, sizeof(data));

    CHECK( mockCommDoesTransmittedDataEqual(appendChecksums("$M10000004,04:78563412#+")) );
}

TEST(gdbRemote, gdbRemoteWriteMemory_ThrowsExceptionIfResponseIsNotOk)
{
    uint32_t data = 0x12345678;
    
    mockCommInitReceiveData(appendChecksums("+$E01#"));
    
    __try_and_catch( gdbRemoteWriteMemory(0x10000004, &data, sizeof(data)) );

    CHECK_EQUAL(badResponseException, getExceptionCode());
    CHECK( mockCommDoesTransmittedDataEqual(appendChecksums("$M10000004,04:78563412#+")) );
}



TEST(gdbRemote, gdbRemoteSingleStep_SendsStepCommandAndReceivesExpectedTResult)
{
    mockCommInitReceiveData(appendChecksums("+$T01#"));
    
    gdbRemoteSingleStep();

    CHECK( mockCommDoesTransmittedDataEqual(appendChecksums("$s#+")) );
}

TEST(gdbRemote, gdbRemoteSingleStep_ThrowsExceptionIfResponseIsNotTPacket)
{
    mockCommInitReceiveData(appendChecksums("+$S01#"));
    
    __try_and_catch( gdbRemoteSingleStep() );

    CHECK_EQUAL(badResponseException, getExceptionCode());
    CHECK( mockCommDoesTransmittedDataEqual(appendChecksums("$s#+")) );
}

/* Copyright 2014 Adam Green (http://mbed.org/users/AdamGreen/)

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published
   by the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.   
*/
#include <string.h>

extern "C"
{
    #include <mockSock.h>
    #include <SocketIComm.h>
}

// Include C++ headers for test harness.
#include "CppUTest/TestHarness.h"

TEST_GROUP(SockIComm)
{
    IComm* m_pComm;
    void setup()
    {
        m_pComm = NULL;
        mockSock_Init(5);
    }

    void teardown()
    {
        CHECK_EQUAL(noException, getExceptionCode());
        SocketIComm_Uninit(m_pComm);
        mockSock_Uninit();
    }
};


TEST(SockIComm, BasicInit_ShouldReturnNonNull)
{
    m_pComm = SocketIComm_Init(SOCKET_ICOMM_DEFAULT_PORT, NULL);
    CHECK(m_pComm != NULL);
}

TEST(SockIComm, FailSocketCallDuringInit_ShouldThrow)
{
    mockSock_socketSetReturn(-1);
        __try_and_catch( m_pComm = SocketIComm_Init(SOCKET_ICOMM_DEFAULT_PORT, NULL) );
    CHECK_EQUAL(socketException, getExceptionCode());
    clearExceptionCode();
}

TEST(SockIComm, FailBindCallDuringInit_ShouldThrow)
{
    mockSock_bindSetReturn(-1);
        __try_and_catch( m_pComm = SocketIComm_Init(SOCKET_ICOMM_DEFAULT_PORT, NULL) );
    CHECK_EQUAL(socketException, getExceptionCode());
    clearExceptionCode();
}

TEST(SockIComm, FailListenCallDuringInit_ShouldThrow)
{
    mockSock_listenSetReturn(-1);
        __try_and_catch( m_pComm = SocketIComm_Init(SOCKET_ICOMM_DEFAULT_PORT, NULL) );
    CHECK_EQUAL(socketException, getExceptionCode());
    clearExceptionCode();
}

TEST(SockIComm, ShouldStopRun_AlwaysReturnFALSE)
{
    m_pComm = SocketIComm_Init(SOCKET_ICOMM_DEFAULT_PORT, NULL);
    CHECK_FALSE(IComm_ShouldStopRun(m_pComm));
}



TEST(SockIComm, FailAcceptCall_HasReceiveData_ShouldThrow)
{
    m_pComm = SocketIComm_Init(SOCKET_ICOMM_DEFAULT_PORT, NULL);
    mockSock_acceptSetReturn(-1);
        __try_and_catch( IComm_HasReceiveData(m_pComm) );
    CHECK_EQUAL(socketException, getExceptionCode());
    clearExceptionCode();
}

TEST(SockIComm, FailSelectCall_HasReceiveData_ShouldThrow)
{
    m_pComm = SocketIComm_Init(SOCKET_ICOMM_DEFAULT_PORT, NULL);
    mockSock_selectSetReturn(-1);
        __try_and_catch( IComm_HasReceiveData(m_pComm) );
    CHECK_EQUAL(socketException, getExceptionCode());
    clearExceptionCode();
}

TEST(SockIComm, HasReceiveData_SelectReturn1_ShouldReturnTrue)
{
    m_pComm = SocketIComm_Init(SOCKET_ICOMM_DEFAULT_PORT, NULL);
    mockSock_selectSetReturn(1);
    CHECK_TRUE(IComm_HasReceiveData(m_pComm));
}

TEST(SockIComm, HasReceiveData_SelectReturn0_ShouldReturnTrue)
{
    m_pComm = SocketIComm_Init(SOCKET_ICOMM_DEFAULT_PORT, NULL);
    mockSock_selectSetReturn(0);
    CHECK_FALSE(IComm_HasReceiveData(m_pComm));
}


static int g_waitingCallbackCalled = 0;
void testWaitingCallback(void)
{
    g_waitingCallbackCalled = 1;
}

TEST(SockIComm, HasReceiveData_ShouldCallWaitingCallback)
{
    m_pComm = SocketIComm_Init(SOCKET_ICOMM_DEFAULT_PORT, testWaitingCallback);
    mockSock_selectSetReturn(0);
    g_waitingCallbackCalled = 0;
        IComm_HasReceiveData(m_pComm);
    CHECK_TRUE(g_waitingCallbackCalled);
}

TEST(SockIComm, HasReceiveData_CallTwice_ShouldOnlyCallWaitingCallbackOnce)
{
    m_pComm = SocketIComm_Init(SOCKET_ICOMM_DEFAULT_PORT, testWaitingCallback);
    mockSock_selectSetReturn(0);
    g_waitingCallbackCalled = 0;
        IComm_HasReceiveData(m_pComm);
    CHECK_TRUE(g_waitingCallbackCalled);

    g_waitingCallbackCalled = 0;
        IComm_HasReceiveData(m_pComm);
    CHECK_FALSE(g_waitingCallbackCalled);
}



TEST(SockIComm, ReceiveChar_RecvReturnNegativeOne_ShouldThrow)
{
    m_pComm = SocketIComm_Init(SOCKET_ICOMM_DEFAULT_PORT, testWaitingCallback);
    mockSock_selectSetReturn(1);
    mockSock_recvSetBuffer("a", 1);
    mockSock_recvSetReturnValues(-1, -1, -1, -1);
        __try_and_catch( IComm_ReceiveChar(m_pComm) );
    CHECK_EQUAL(socketException, getExceptionCode());
    clearExceptionCode();
}

TEST(SockIComm, ReceiveChar_RecvSingleChar)
{
    m_pComm = SocketIComm_Init(SOCKET_ICOMM_DEFAULT_PORT, testWaitingCallback);
    mockSock_selectSetReturn(1);
    mockSock_recvSetBuffer("a", 1);
    mockSock_recvSetReturnValues(1, -1, -1, -1);
    char c = IComm_ReceiveChar(m_pComm);
    CHECK_EQUAL('a', c);
}

TEST(SockIComm, ReceiveChar_RecvReturnZero_ShouldReturnZero_AndWaitAgainOnNextCall)
{
    m_pComm = SocketIComm_Init(SOCKET_ICOMM_DEFAULT_PORT, testWaitingCallback);
    mockSock_selectSetReturn(1);
    mockSock_recvSetBuffer("a", 1);
    mockSock_recvSetReturnValues(0, -1, -1, -1);
    g_waitingCallbackCalled = 0;
        IComm_ReceiveChar(m_pComm);
    CHECK_TRUE(g_waitingCallbackCalled);

    mockSock_selectSetReturn(1);
    mockSock_recvSetBuffer("a", 1);
    mockSock_recvSetReturnValues(1, -1, -1, -1);
    g_waitingCallbackCalled = 0;
        IComm_ReceiveChar(m_pComm);
    CHECK_TRUE(g_waitingCallbackCalled);
}

TEST(SockIComm, ReceiveChar_ShouldCallWaitingCallback)
{
    m_pComm = SocketIComm_Init(SOCKET_ICOMM_DEFAULT_PORT, testWaitingCallback);
    mockSock_recvSetBuffer("a", 1);
    mockSock_recvSetReturnValues(1, -1, -1, -1);
    g_waitingCallbackCalled = 0;
        IComm_ReceiveChar(m_pComm);
    CHECK_TRUE(g_waitingCallbackCalled);
}


TEST(SockIComm, SendChar_FailSend_ShouldThrow)
{
    m_pComm = SocketIComm_Init(SOCKET_ICOMM_DEFAULT_PORT, testWaitingCallback);
    mockSock_sendFailIteration(1);
        __try_and_catch( IComm_SendChar(m_pComm, 'z') );
    CHECK_EQUAL(socketException, getExceptionCode());
    clearExceptionCode();
}

TEST(SockIComm, SendChar_VerifySuccessfullySentBytes)
{
    m_pComm = SocketIComm_Init(SOCKET_ICOMM_DEFAULT_PORT, testWaitingCallback);
        IComm_SendChar(m_pComm, 'x');
        IComm_SendChar(m_pComm, 'y');
        IComm_SendChar(m_pComm, 'z');
    STRCMP_EQUAL("xyz", mockSock_sendData());
}

TEST(SockIComm, SendChar_ShouldCallWaitingCallback)
{
    m_pComm = SocketIComm_Init(SOCKET_ICOMM_DEFAULT_PORT, testWaitingCallback);
    g_waitingCallbackCalled = 0;
        IComm_SendChar(m_pComm, 'z');
    CHECK_TRUE(g_waitingCallbackCalled);
}
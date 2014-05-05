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
#include <mockSock.h>
#include <netdb.h>
#include <SocketIComm.h>
#include <string.h>


#define FALSE 0
#define TRUE  1


/* Implementation of IComm interface. */
typedef struct SocketIComm SocketIComm;

static int  hasReceiveData(IComm* pComm);
static int  receiveChar(IComm* pComm);
static void sendChar(IComm* pComm, int character);
static int  shouldStopRun(IComm* pComm);
static int  isGdbConnected(IComm* pComm);

ICommVTable g_icommVTable = {hasReceiveData, receiveChar, sendChar, shouldStopRun, isGdbConnected};

struct SocketIComm
{
    ICommVTable* pVTable;
    void         (*waitingConnectCallback)(void);
    int          listenSocket;
    int          gdbSocket;
} g_comm = {&g_icommVTable, NULL, -1, -1};


static void createListenSocket(SocketIComm* pThis);
static void bindListenSocket(SocketIComm* pThis, uint16_t gdbPort);
static void allowBindToReuseAddress(SocketIComm* pThis);
static void listenOnSocket(SocketIComm* pThis);
static void waitForGdbConnectIfNecessary(SocketIComm* pThis);
static int socketHasDataToRead(int socket);
static int receiveNextCharFromGdb(SocketIComm* pThis);


__throws IComm* SocketIComm_Init(uint16_t gdbPort, void (*waitingConnectCallback)(void))
{
    SocketIComm* pThis = &g_comm;
    
    __try
    {
        createListenSocket(pThis);
        bindListenSocket(pThis, gdbPort);
        listenOnSocket(pThis);
        pThis->waitingConnectCallback = waitingConnectCallback;
    }
    __catch
    {
        SocketIComm_Uninit((IComm*)pThis);
        __rethrow;
    }
    
    return (IComm*)pThis;
}

static void createListenSocket(SocketIComm* pThis)
{
    pThis->listenSocket = socket(PF_INET, SOCK_STREAM, 0);
    if (pThis->listenSocket == -1)
        __throw(socketException);
}

static void bindListenSocket(SocketIComm* pThis, uint16_t gdbPort)
{
    int                result = -1;
    struct sockaddr_in bindAddress;

    allowBindToReuseAddress(pThis);
    
    memset(&bindAddress, 0, sizeof(bindAddress));
    bindAddress.sin_family = AF_INET;
    bindAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    bindAddress.sin_port = htons(gdbPort);
    result = bind(pThis->listenSocket, (struct sockaddr *)&bindAddress, sizeof(bindAddress));
    if (result == -1)
        __throw(socketException);
}

static void allowBindToReuseAddress(SocketIComm* pThis)
{
    int optionValue = 1;
    setsockopt(pThis->listenSocket, SOL_SOCKET, SO_REUSEADDR, &optionValue, sizeof(optionValue));
}

static void listenOnSocket(SocketIComm* pThis)
{
    int result = listen(pThis->listenSocket, 0);
    if (result == -1)
        __throw(socketException);
}


void SocketIComm_Uninit(IComm* pComm)
{
    SocketIComm* pThis = (SocketIComm*)pComm;

    if (!pThis)
        return;

    pThis->waitingConnectCallback = NULL;
    if (pThis->gdbSocket != -1)
    {
        close(pThis->gdbSocket);
        pThis->gdbSocket = -1;
    }
    if (pThis->listenSocket != -1)
    {
        close(pThis->listenSocket);
        pThis->listenSocket = -1;
    }
}



/* IComm Interface Implementation. */
static int hasReceiveData(IComm* pComm)
{
    int            hasData = FALSE;
    SocketIComm*   pThis = (SocketIComm*)pComm;
    
    __try
    {
        waitForGdbConnectIfNecessary(pThis);
        hasData = socketHasDataToRead(pThis->gdbSocket);
    }
    __catch
    {
        return FALSE;
    }
    return hasData;
}

static void waitForGdbConnectIfNecessary(SocketIComm* pThis)
{
    struct sockaddr_in remoteAddress;
    socklen_t          remoteAddressSize = sizeof(remoteAddress);

    if (pThis->gdbSocket != -1)
        return;
    if (pThis->waitingConnectCallback)
        pThis->waitingConnectCallback();

    pThis->gdbSocket = accept(pThis->listenSocket, (struct sockaddr*)&remoteAddress, &remoteAddressSize);
    if (pThis->gdbSocket == -1)
        __throw(socketException);
}

static int socketHasDataToRead(int socket)
{
    int            result = -1;
    struct timeval zeroTimeout = {0, 0};
    fd_set         readSet;

    FD_ZERO(&readSet);
    FD_SET(socket, &readSet);
    result = select(socket + 1, &readSet, NULL, NULL, &zeroTimeout);
    if (result == -1)
        __throw(socketException);
    return result;
}

static int receiveChar(IComm* pComm)
{
    waitForGdbConnectIfNecessary((SocketIComm*)pComm);
    return receiveNextCharFromGdb((SocketIComm*)pComm);
}

static int receiveNextCharFromGdb(SocketIComm* pThis)
{
    int  result = -1;
    char c = 0;

    result = recv(pThis->gdbSocket, &c, sizeof(c), 0);
    if (result == -1)
    {
        __throw(socketException);
    }
    else if (result == 0)
    {
        /* GDB has closed its side of the socket connection. */
        close(pThis->gdbSocket);
        pThis->gdbSocket = -1;
        return 0;
    }
    return c;
}

static void sendChar(IComm* pComm, int character)
{
    int          result = -1;
    SocketIComm* pThis = (SocketIComm*)pComm;
    char         c = (char)character;
    
    waitForGdbConnectIfNecessary(pThis);
    result = send(pThis->gdbSocket, &c, sizeof(c), 0);
    if (result == -1)
        __throw(socketException);
}

static int shouldStopRun(IComm* pComm)
{
    return FALSE;
}

static int  isGdbConnected(IComm* pComm)
{
    SocketIComm*   pThis = (SocketIComm*)pComm;
    
    if (pThis->gdbSocket != -1)
        return TRUE;
        
    return socketHasDataToRead(pThis->listenSocket);
}

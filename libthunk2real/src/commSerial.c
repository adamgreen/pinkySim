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
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include "commSerial.h"


/* Set SERIAL_LOG to 1 to have all serial I/O logged to serial.log file and set to 0 otherwise. */
#define SERIAL_LOG 0


typedef struct CommSerial
{
    FILE*          pLogFile;
    struct termios origTerm;
    int            file;
    int            origTermValid;
    int            writeLast;
    uint32_t       secTimeout;
    uint32_t       usecTimeout;
} CommSerial;


CommSerial g_serial;


static int openMbedSerial(void);


__throws void commSerialInit(const char* pDevicePath, uint32_t baudRate, uint32_t msecTimeout)
{
    int            result = -1;
    struct termios newTerm;

    memset(&g_serial, 0, sizeof(g_serial));

    if (SERIAL_LOG)
    {
        g_serial.pLogFile = fopen("serial.log", "w");
        g_serial.writeLast = 0;
    }

    if (pDevicePath)
        g_serial.file = open(pDevicePath, O_RDWR | O_NONBLOCK);
    else
        g_serial.file = openMbedSerial();
    if (g_serial.file == -1)
    {
        perror("error: Failed to open serial port");
        __throw(serialException);
    }

    /* Remember the desired timeout for reads/writes */
    g_serial.secTimeout = msecTimeout / 1000;
    msecTimeout = msecTimeout % 1000;
    g_serial.usecTimeout = msecTimeout * 1000;

    /* Configure serial port settings. */
    result = tcgetattr(g_serial.file, &g_serial.origTerm);
    if (result == -1)
        __throw(serialException);
    g_serial.origTermValid = 1;
    newTerm = g_serial.origTerm;

    /* Set the baud rate. */
    result = cfsetspeed(&newTerm, baudRate);
    if (result == -1)
    {
        perror("error: Failed to set baud rate");
        __throw(serialException);
    }

    /* Use Non-canonical mode. */
    cfmakeraw(&newTerm);

    /* No input or output mapping. */
    newTerm.c_iflag = 0;
    newTerm.c_oflag = 0;


    /* Configure for 8n1 format and disable modem flow control. */
    newTerm.c_cflag = CS8 | CREAD | CLOCAL;

    /* Set MIN characters and TIMEout for non-canonical mode. */
    newTerm.c_cc[VMIN] = 0;
    newTerm.c_cc[VTIME] = 0;

    result = tcflush(g_serial.file, TCIOFLUSH);
    if (result != 0)
        __throw(serialException);

    result = tcsetattr(g_serial.file, TCSANOW, &newTerm);
    if (result == -1)
    {
        perror("error: Failed to configure serial port");
        __throw(serialException);
    }

    /* This might only be required due to the mbed CDC firmware. */
    usleep(100000);
}

/* Find the first device named /dev/tty.usbmodem* on OS X and open it. */
static int openMbedSerial(void)
{
    int            serialFile = -1;
    DIR*           pDir = NULL;
    struct dirent* pDirEntry = NULL;

    pDir = opendir("/dev");
    if (!pDir)
        __throw(serialException);

    while (NULL != (pDirEntry = readdir(pDir)))
    {
        if (pDirEntry->d_name == strcasestr(pDirEntry->d_name, "tty.usbmodem"))
        {
            char pathname[128];

            snprintf(pathname, sizeof(pathname), "/dev/%s", pDirEntry->d_name);
            serialFile = open(pathname, O_RDWR | O_NONBLOCK);
            break;
        }
    }

    if (pDir)
        closedir(pDir);

    return serialFile;
}



void commSerialUninit()
{
    if (g_serial.origTermValid)
        tcsetattr(g_serial.file, TCSAFLUSH, &g_serial.origTerm);
    if (g_serial.file != -1)
        close(g_serial.file);
    if (SERIAL_LOG && g_serial.pLogFile)
    {
        fclose(g_serial.pLogFile);
        g_serial.pLogFile = NULL;
    }
}



/* comm.h interface implementation. */
uint32_t commHasReceiveData(void)
{
    int            selectResult = -1;
    fd_set         readfds;
    fd_set         errorfds;
    struct timeval timeOut = {0, 0};

    FD_ZERO(&readfds);
    FD_ZERO(&errorfds);
    FD_SET(g_serial.file, &readfds);

    selectResult = select(g_serial.file+1, &readfds, NULL, &errorfds, &timeOut);
    if (selectResult == -1)
        __throws(serialException);
    if (selectResult == 0)
        return 0;
    else
        return 1;
}



int commReceiveChar(void)
{
    ssize_t        bytesRead = 0;
    uint8_t        byte = 0;
    int            selectResult = -1;
    fd_set         readfds;
    fd_set         errorfds;
    struct timeval timeOut = {g_serial.secTimeout, g_serial.usecTimeout};

    FD_ZERO(&readfds);
    FD_ZERO(&errorfds);
    FD_SET(g_serial.file, &readfds);

    selectResult = select(g_serial.file+1, &readfds, NULL, &errorfds, &timeOut);
    if (selectResult == -1)
        __throw(serialException);
    if (selectResult == 0)
        __throw(timeoutException);

    bytesRead = read(g_serial.file, &byte, sizeof(byte));
    if (bytesRead == -1)
        __throw(serialException);

    if (SERIAL_LOG && g_serial.pLogFile)
    {
        if (g_serial.writeLast)
            fwrite("\n[r]", 1, 4, g_serial.pLogFile);
        g_serial.writeLast = 0;
        fwrite(&byte, 1, 1, g_serial.pLogFile);
        fflush(g_serial.pLogFile);
    }

    return (int)byte;
}



void commSendChar(int character)
{
    uint8_t        byte = (uint8_t)character;
    ssize_t        bytesWritten = 0;
    int            selectResult = -1;
    fd_set         writefds;
    fd_set         errorfds;
    struct timeval timeOut = {g_serial.secTimeout, g_serial.usecTimeout};

    FD_ZERO(&writefds);
    FD_ZERO(&errorfds);
    FD_SET(g_serial.file, &writefds);

    selectResult = select(g_serial.file+1, NULL, &writefds, &errorfds, &timeOut);
    if (selectResult == -1)
        __throw(serialException);
    if (selectResult == 0)
        __throw(timeoutException);

    bytesWritten = write(g_serial.file, &byte, sizeof(byte));
    if (bytesWritten != sizeof(byte))
        __throw(serialException);
    if (SERIAL_LOG && g_serial.pLogFile)
    {
        if (!g_serial.writeLast)
            fwrite("\n[w]", 1, 4, g_serial.pLogFile);
        g_serial.writeLast = 1;
        fwrite(&byte, 1, 1, g_serial.pLogFile);
        fflush(g_serial.pLogFile);
    }
}

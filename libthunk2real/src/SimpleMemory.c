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
#include <gdbRemote.h>
#include <SimpleMemory.h>
#include <string.h>
#include "commSerial.h"


/* Forward Declarations */
typedef struct SimpleMemory SimpleMemory;
static uint32_t read32(IMemory* pThis, uint32_t address);
static uint16_t read16(IMemory* pMem, uint32_t address);
static uint8_t read8(IMemory* pMem, uint32_t address);
static void write32(IMemory* pMem, uint32_t address, uint32_t value);
static void write16(IMemory* pMem, uint32_t address, uint16_t value);
static void write8(IMemory* pMem, uint32_t address, uint8_t value);

static IMemoryVTable g_vTable = {read32, read16, read8, write32, write16, write8};

struct SimpleMemory
{
    IMemoryVTable* pVTable;
};

static SimpleMemory g_object;
static int          g_initComplete;



IMemory* SimpleMemory_Init(void)
{
    memset(&g_object, 0, sizeof(g_object));
    g_object.pVTable = &g_vTable;

    if (!g_initComplete)
    {
        commSerialInit(NULL, 230400, 1000);
        commSendChar('+');
        g_initComplete = 1;
    }

    return (IMemory*)&g_object;
}

void SimpleMemory_SetMemory(IMemory* pMem, uint32_t address, uint32_t value, int readOnly)
{
    write32(pMem, address, value);
    return;
}


/* IMemory interface methods */
static uint32_t read32(IMemory* pMem, uint32_t address)
{
    uint32_t value;

    gdbRemoteReadMemory(address, &value, sizeof(value));
    return value;
}

static uint16_t read16(IMemory* pMem, uint32_t address)
{
    uint16_t value;

    gdbRemoteReadMemory(address, &value, sizeof(value));
    return value;
}

static uint8_t read8(IMemory* pMem, uint32_t address)
{
    uint8_t value;

    gdbRemoteReadMemory(address, &value, sizeof(value));
    return value;
}


static void write32(IMemory* pMem, uint32_t address, uint32_t value)
{
    gdbRemoteWriteMemory(address, &value, sizeof(value));
}

static void write16(IMemory* pMem, uint32_t address, uint16_t value)
{
    gdbRemoteWriteMemory(address, &value, sizeof(value));
}

static void write8(IMemory* pMem, uint32_t address, uint8_t value)
{
    gdbRemoteWriteMemory(address, &value, sizeof(value));
}

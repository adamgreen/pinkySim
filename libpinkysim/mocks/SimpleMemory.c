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
#include <SimpleMemory.h>


/* Forward Declarations */
typedef struct SimpleMemory SimpleMemory;
static uint32_t read32(IMemory* pThis, uint32_t address);
static uint16_t read16(IMemory* pMem, uint32_t address);
static uint8_t read8(IMemory* pMem, uint32_t address);
static uint32_t read(SimpleMemory* pThis, uint32_t address);
static void write32(IMemory* pMem, uint32_t address, uint32_t value);
static void write16(IMemory* pMem, uint32_t address, uint16_t value);
static void write8(IMemory* pMem, uint32_t address, uint8_t value);
static void write(SimpleMemory* pThis, uint32_t address, uint32_t alignedValue, uint32_t mask);

static IMemoryVTable g_vTable = {read32, read16, read8, write32, write16, write8};

typedef struct MemoryEntry
{
    int      readOnly;
    uint32_t address;
    uint32_t value;
} MemoryEntry;

struct SimpleMemory
{
    IMemoryVTable* pVTable;
    size_t         entryCount;
    MemoryEntry    entries[SIMPLE_MEMORY_REGION_COUNT];
};

static SimpleMemory g_object;



IMemory* SimpleMemory_Init(void)
{
    memset(&g_object, 0, sizeof(g_object));
    g_object.pVTable = &g_vTable;
    
    return (IMemory*)&g_object;
}

void SimpleMemory_SetMemory(IMemory* pMem, uint32_t address, uint32_t value, int readOnly)
{
    SimpleMemory* pThis = (SimpleMemory*)pMem;
    
    if (pThis->entryCount >= sizeof(pThis->entries)/sizeof(pThis->entries[0]))
        __throw(outOfMemoryException);
    
    pThis->entries[pThis->entryCount].address = address;
    pThis->entries[pThis->entryCount].value = value;
    pThis->entries[pThis->entryCount].readOnly = readOnly;
    pThis->entryCount++;
}


/* IMemory interface methods */
static uint32_t read32(IMemory* pMem, uint32_t address)
{
    return read((SimpleMemory*)pMem, address);
}

static uint16_t read16(IMemory* pMem, uint32_t address)
{
    return (uint16_t)read((SimpleMemory*)pMem, address);
}

static uint8_t read8(IMemory* pMem, uint32_t address)
{
    return (uint8_t)read((SimpleMemory*)pMem, address);
}

static uint32_t read(SimpleMemory* pThis, uint32_t address)
{
    uint32_t alignedAddress = address & 0xFFFFFFFC;
    size_t   i;
    
    for (i = 0 ; i < pThis->entryCount ; i++)
    {
        if (pThis->entries[i].address == alignedAddress)
        {
            uint32_t value = pThis->entries[i].value;
            return (value >> 8 * (address - alignedAddress));
        }
    }
    __throw(busErrorException);
}

static void write32(IMemory* pMem, uint32_t address, uint32_t value)
{
    write((SimpleMemory*)pMem, address, value, 0xFFFFFFFF);
}

static void write16(IMemory* pMem, uint32_t address, uint16_t value)
{
    write((SimpleMemory*)pMem, address, (uint32_t)value, 0xFFFF);
}

static void write8(IMemory* pMem, uint32_t address, uint8_t value)
{
    write((SimpleMemory*)pMem, address, (uint32_t)value, 0xFF);
}

static void write(SimpleMemory* pThis, uint32_t address, uint32_t value, uint32_t mask)
{
    uint32_t      alignedAddress = address & 0xFFFFFFFC;
    size_t        i;
    
    for (i = 0 ; i < pThis->entryCount ; i++)
    {
        if (pThis->entries[i].address == alignedAddress && !pThis->entries[i].readOnly)
        {
            value <<= 8 * (address - alignedAddress);
            mask <<= 8 * (address - alignedAddress);
            pThis->entries[i].value = value | (pThis->entries[i].value & ~mask);
            return;
        }
    }
    __throw(busErrorException);
}

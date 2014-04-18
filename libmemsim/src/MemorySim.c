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
#include <string.h>
#include <MemorySim.h>
#include <MallocFailureInject.h>


#define READING 0
#define WRITING  1

/* Forward Declarations */
typedef struct MemorySim MemorySim;
typedef struct MemoryRegion MemoryRegion;

static uint32_t read32(IMemory* pMemory, uint32_t address);
static uint16_t read16(IMemory* pMemory, uint32_t address);
static uint8_t read8(IMemory* pMemory, uint32_t address);
static void write32(IMemory* pMemory, uint32_t address, uint32_t value);
static void write16(IMemory* pMemory, uint32_t address, uint16_t value);
static void write8(IMemory* pMemory, uint32_t address, uint8_t value);

static void* getDataPointer(MemorySim* pThis, uint32_t address, uint32_t size, int writeMode);
static MemoryRegion* findMatchingRegion(MemorySim* pThis, uint32_t address, uint32_t size);

static IMemoryVTable g_vTable = {read32, read16, read8, write32, write16, write8};

struct MemoryRegion
{
    struct MemoryRegion* pNext;
    uint8_t*             pData;
    int                  readOnly;
    uint32_t             baseAddress;
    uint32_t             size;
};

struct MemorySim
{
    IMemoryVTable* pVTable;
    MemoryRegion*  pHeadRegion;
    MemoryRegion*  pTailRegion;
};

static MemorySim g_object;


IMemory* MemorySim_Init(void)
{
    memset(&g_object, 0, sizeof(g_object));
    g_object.pVTable = &g_vTable;
    return (IMemory*)&g_object;
}

void MemorySim_Uninit(IMemory* pMemory)
{
    MemorySim*    pThis = (MemorySim*)pMemory;
    MemoryRegion* pCurr;
    
    if (!pThis)
        return;

    pCurr = pThis->pHeadRegion;
    while (pCurr)
    {
        MemoryRegion* pNext = pCurr->pNext;
        free(pCurr->pData);
        free(pCurr);
        pCurr = pNext;
    }
    pThis->pHeadRegion = pThis->pTailRegion = NULL;
}

void MemorySim_CreateRegion(IMemory* pMemory, uint32_t baseAddress, uint32_t size)
{
    MemorySim*             pThis = (MemorySim*)pMemory;
    MemoryRegion* volatile pRegion = NULL;

    __try
    {
        pRegion = malloc(sizeof(*pRegion));
        if (!pRegion)
            __throw(outOfMemoryException);
        pRegion->pNext = NULL;
        pRegion->readOnly = 0;
        pRegion->baseAddress = baseAddress;
        pRegion->size = size;
        pRegion->pData = malloc(size);
        if (!pRegion->pData)
            __throw(outOfMemoryException);
        memset(pRegion->pData, 0, size);

        if (!pThis->pTailRegion)
        {
            assert(!pThis->pHeadRegion);
            pThis->pHeadRegion = pRegion;
            pThis->pTailRegion = pRegion;
        }
        else
        {
            pThis->pTailRegion->pNext = pRegion;
        }
    }
    __catch
    {
        if (pRegion)
        {
            free(pRegion->pData);
            free(pRegion);
        }
    }
}

void MemorySim_MakeRegionReadOnly(IMemory* pMemory, uint32_t baseAddress)
{
    MemorySim* pThis = (MemorySim*)pMemory;
    MemoryRegion* pRegion = findMatchingRegion(pThis, baseAddress, 1);
    pRegion->readOnly = 1;
}


/* IMemory interface methods */
static uint32_t read32(IMemory* pMemory, uint32_t address)
{
    return *(uint32_t*)getDataPointer((MemorySim*)pMemory, address, sizeof(uint32_t), READING);
}

static uint16_t read16(IMemory* pMemory, uint32_t address)
{
    return *(uint16_t*)getDataPointer((MemorySim*)pMemory, address, sizeof(uint16_t), READING);
}

static uint8_t read8(IMemory* pMemory, uint32_t address)
{
    return *(uint8_t*)getDataPointer((MemorySim*)pMemory, address, sizeof(uint8_t), READING);
}

static void write32(IMemory* pMemory, uint32_t address, uint32_t value)
{
    *(uint32_t*)getDataPointer((MemorySim*)pMemory, address, sizeof(uint32_t), WRITING) = value;
}

static void write16(IMemory* pMemory, uint32_t address, uint16_t value)
{
    *(uint16_t*)getDataPointer((MemorySim*)pMemory, address, sizeof(uint16_t), WRITING) = value;
}

static void write8(IMemory* pMemory, uint32_t address, uint8_t value)
{
    *(uint8_t*)getDataPointer((MemorySim*)pMemory, address, sizeof(uint8_t), WRITING) = value;
}


static void* getDataPointer(MemorySim* pThis, uint32_t address, uint32_t size, int writeMode)
{
    MemoryRegion* pRegion = findMatchingRegion(pThis, address, size);
    uint32_t regionOffset = address - pRegion->baseAddress;
    if (writeMode && pRegion->readOnly)
        __throw(busErrorException);
    return pRegion->pData + regionOffset;
}

static MemoryRegion* findMatchingRegion(MemorySim* pThis, uint32_t address, uint32_t size)
{
    MemoryRegion* pCurr = pThis->pHeadRegion;
    
    while (pCurr)
    {
        MemoryRegion* pNext = pCurr->pNext;
        if (address >= pCurr->baseAddress && address + size <= pCurr->baseAddress + pCurr->size)
            return pCurr;
        pCurr = pNext;
    }
    __throw(busErrorException);
}

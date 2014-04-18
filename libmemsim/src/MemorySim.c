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

static void freeRegion(MemoryRegion* pRegion);
static void* throwingZeroedMalloc(size_t size);
static void addRegionToTail(MemorySim* pThis, MemoryRegion* pRegion);
static MemoryRegion* findMatchingRegion(MemorySim* pThis, uint32_t address, uint32_t size);
static void copyImageToRegion(IMemory* pMemory, const void* pFlashImage, uint32_t flashImageSize);
static void freeLastRegion(MemorySim* pThis);
static void* getDataPointer(MemorySim* pThis, uint32_t address, uint32_t size, int writeMode);

static uint32_t read32(IMemory* pMemory, uint32_t address);
static uint16_t read16(IMemory* pMemory, uint32_t address);
static uint8_t read8(IMemory* pMemory, uint32_t address);
static void write32(IMemory* pMemory, uint32_t address, uint32_t value);
static void write16(IMemory* pMemory, uint32_t address, uint16_t value);
static void write8(IMemory* pMemory, uint32_t address, uint8_t value);

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
        freeRegion(pCurr);
        pCurr = pNext;
    }
    pThis->pHeadRegion = pThis->pTailRegion = NULL;
}

static void freeRegion(MemoryRegion* pRegion)
{
    if (!pRegion)
        return;
        
    free(pRegion->pData);
    free(pRegion);
}


void MemorySim_CreateRegion(IMemory* pMemory, uint32_t baseAddress, uint32_t size)
{
    MemorySim*             pThis = (MemorySim*)pMemory;
    MemoryRegion* volatile pRegion = NULL;

    __try
    {
        pRegion = throwingZeroedMalloc(sizeof(*pRegion));
        pRegion->baseAddress = baseAddress;
        pRegion->size = size;
        pRegion->pData = throwingZeroedMalloc(size);
        addRegionToTail(pThis, pRegion);
    }
    __catch
    {
        freeRegion(pRegion);
        __rethrow;
    }
}

static void* throwingZeroedMalloc(size_t size)
{
    void* pvAlloc = malloc(size);
    if (!pvAlloc)
        __throw(outOfMemoryException);
    memset(pvAlloc, 0, size);
    return pvAlloc;
}

static void addRegionToTail(MemorySim* pThis, MemoryRegion* pRegion)
{
    if (!pThis->pTailRegion)
        pThis->pHeadRegion = pThis->pTailRegion = pRegion;
    else
        pThis->pTailRegion->pNext = pRegion;
}


void MemorySim_MakeRegionReadOnly(IMemory* pMemory, uint32_t baseAddress)
{
    MemorySim* pThis = (MemorySim*)pMemory;
    MemoryRegion* pRegion = findMatchingRegion(pThis, baseAddress, 1);
    pRegion->readOnly = 1;
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


__throws void MemorySim_CreateRegionsFromFlashImage(IMemory* pMemory, const void* pFlashImage, uint32_t flashImageSize)
{
    MemorySim*      pThis = (MemorySim*)pMemory;
    
    if (flashImageSize < sizeof(uint32_t))
        __throw(bufferOverrunException);

    MemorySim_CreateRegion(pMemory, FLASH_BASE_ADDRESS, flashImageSize);
    copyImageToRegion(pMemory, pFlashImage, flashImageSize);
    MemorySim_MakeRegionReadOnly(pMemory, FLASH_BASE_ADDRESS);

    __try
    {
        uint32_t endRAMAddress = *(uint32_t*)pFlashImage;
        uint32_t baseRAMAddress =  endRAMAddress & RAM_ADDRESS_MASK;
        MemorySim_CreateRegion(pMemory, baseRAMAddress, endRAMAddress - baseRAMAddress);
    }
    __catch
    {
        freeLastRegion(pThis);
        __rethrow;
    }
}

static void copyImageToRegion(IMemory* pMemory, const void* pFlashImage, uint32_t flashImageSize)
{
    const uint32_t* pSrcWord = (uint32_t*)pFlashImage;
    uint32_t        address = FLASH_BASE_ADDRESS;
    const uint8_t*  pSrcByte;

    while (flashImageSize > sizeof(uint32_t))
    {
        write32(pMemory, address, *pSrcWord++);
        address += sizeof(uint32_t);
        flashImageSize -= sizeof(uint32_t);
    }
    
    pSrcByte = (const uint8_t*)pSrcWord;
    while (flashImageSize--)
        write8(pMemory, address++, *pSrcByte++);
}

static void freeLastRegion(MemorySim* pThis)
{
    MemoryRegion* pPrev = NULL;
    MemoryRegion* pCurr = pThis->pHeadRegion;
    
    while (pCurr && pCurr->pNext)
    {
        pPrev = pCurr;
        pCurr = pCurr->pNext;
    }
    if (!pPrev)
        pThis->pHeadRegion = pThis->pTailRegion = NULL;
    else
        pPrev->pNext = NULL;
    free(pCurr->pData);
    free(pCurr);
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

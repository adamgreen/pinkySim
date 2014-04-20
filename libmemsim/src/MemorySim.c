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

/* Bit that can be set in Watchpoint::type field to flag as breakpoint and not watchpoint. */
#define WATCHPOINT_BREAKPOINT (WatchpointType)((1 << 31) | WATCHPOINT_READ)

/* Memory access types are based on watchpoint types so that watchpoint can be used as mask. */
#define AccessType WatchpointType
#define READING    WATCHPOINT_READ
#define WRITING    WATCHPOINT_WRITE

typedef enum MatchResult
{
    FOUND,          /* Found requested value. */
    FOUND_HIGHER,   /* Found a value higher than requested but not exact value requested. */
    NOT_FOUND       /* Walked all entries and found no match or anything higher. */
    
} MatchResult;

/* Forward Declarations */
typedef struct MemorySim MemorySim;
typedef struct MemoryRegion MemoryRegion;
typedef struct Watchpoint Watchpoint;

static void freeRegion(MemoryRegion* pRegion);
static void* throwingZeroedMalloc(size_t size);
static void addRegionToTail(MemorySim* pThis, MemoryRegion* pRegion);
static MemoryRegion* findMatchingRegion(MemorySim* pThis, uint32_t address, uint32_t size);
static void copyImageToRegion(IMemory* pMemory, const void* pFlashImage, uint32_t flashImageSize);
static void freeLastRegion(MemorySim* pThis);
static void setWatchpoint(IMemory* pMemory, uint32_t address, uint32_t size, WatchpointType type);
static MatchResult findMatchingOrHigherWatchpoint(MemoryRegion* pRegion, Watchpoint* pKey, uint32_t* pIndex);
static int compareWatchpoints(const void* pvKey, const void* pvCurr);
static int watchpointsMatch(const Watchpoint* p1, const Watchpoint* p2);
static void growWatchpointArrayIfNeeded(MemoryRegion* pRegion, uint32_t requiredSize);
static void clearWatchpoint(IMemory* pMemory, uint32_t address, uint32_t size, WatchpointType type);
static void* getDataPointer(MemorySim* pThis, uint32_t address, uint32_t size, AccessType type);
static void checkForBreakWatchPoint(MemorySim* pThis, 
                                    MemoryRegion* pRegion,
                                    uint32_t address, uint32_t size, AccessType type);
static int accessInRange(Watchpoint* pWatchpoint, uint32_t startAddress, uint32_t endAddress);

static uint32_t read32(IMemory* pMemory, uint32_t address);
static uint16_t read16(IMemory* pMemory, uint32_t address);
static uint8_t read8(IMemory* pMemory, uint32_t address);
static void write32(IMemory* pMemory, uint32_t address, uint32_t value);
static void write16(IMemory* pMemory, uint32_t address, uint16_t value);
static void write8(IMemory* pMemory, uint32_t address, uint8_t value);

static IMemoryVTable g_vTable = {read32, read16, read8, write32, write16, write8};

struct Watchpoint
{
    WatchpointType type;
    uint32_t       startAddress;
    uint32_t       endAddress;
    
};

struct MemoryRegion
{
    struct MemoryRegion* pNext;
    uint8_t*             pData;
    int                  readOnly;
    uint32_t             baseAddress;
    uint32_t             size;
    Watchpoint*          pWatchpoints;
    uint32_t             watchpointCount;
    uint32_t             watchpointAlloc;
};

struct MemorySim
{
    IMemoryVTable* pVTable;
    int            watchpointEncountered;
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
    
    free(pRegion->pWatchpoints);
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


__throws void MemorySim_SetHardwareBreakpoint(IMemory* pMemory, uint32_t address, uint32_t size)
{
    setWatchpoint(pMemory, address, size, WATCHPOINT_BREAKPOINT);
}

static void setWatchpoint(IMemory* pMemory, uint32_t address, uint32_t size, WatchpointType type)
{
    MemoryRegion* pRegion = findMatchingRegion((MemorySim*)pMemory, address, size);
    uint32_t      endAddress = address + size;
    Watchpoint    watchpoint = {type, address, endAddress};
    uint32_t      i;
    MatchResult   match = findMatchingOrHigherWatchpoint(pRegion, &watchpoint, &i);

    if (match == FOUND)
        return;
    growWatchpointArrayIfNeeded(pRegion, pRegion->watchpointCount + 1);
    memmove(&pRegion->pWatchpoints[i+1],
            &pRegion->pWatchpoints[i],
            sizeof(*pRegion->pWatchpoints) * (pRegion->watchpointCount - i));
    pRegion->pWatchpoints[i] = watchpoint;
    pRegion->watchpointCount++;
}


static MatchResult findMatchingOrHigherWatchpoint(MemoryRegion* pRegion, Watchpoint* pKey, uint32_t* pIndex)
{
    uint32_t           i;
    
    for (i = 0 ; i < pRegion->watchpointCount ; i++)
    {
        int result = compareWatchpoints(pKey, &pRegion->pWatchpoints[i]);
        if (result == 0)
        {
            *pIndex = i;
            return FOUND;
        }
        else if (result < 0)
        {
            *pIndex = i;
            return FOUND_HIGHER;
        }
    }
    *pIndex = i;
    return NOT_FOUND;
}

static int compareWatchpoints(const void* pvKey, const void* pvCurr)
{
    const Watchpoint* pKey = (const Watchpoint*)pvKey;
    const Watchpoint* pCurr = (const Watchpoint*)pvCurr;
    
    if (watchpointsMatch(pKey, pCurr))
        return 0;
    else if (pKey->startAddress < pCurr->startAddress)
        return -1;
    else
        return 1;
}

static int watchpointsMatch(const Watchpoint* p1, const Watchpoint* p2)
{
    return 0 == memcmp(p1, p2, sizeof(*p1));
}

static void growWatchpointArrayIfNeeded(MemoryRegion* pRegion, uint32_t requiredSize)
{
    if (requiredSize > pRegion->watchpointAlloc)
    {
        Watchpoint* pRealloc = realloc(pRegion->pWatchpoints, sizeof(*pRegion->pWatchpoints) * requiredSize);
        if (!pRealloc)
            __throw(outOfMemoryException);
        pRegion->pWatchpoints = pRealloc;
        pRegion->watchpointAlloc = requiredSize;
    }
}


__throws void MemorySim_ClearHardwareBreakpoint(IMemory* pMemory, uint32_t address, uint32_t size)
{
    clearWatchpoint(pMemory, address, size, WATCHPOINT_BREAKPOINT);
}

static void clearWatchpoint(IMemory* pMemory, uint32_t address, uint32_t size, WatchpointType type)
{
    MemoryRegion* pRegion = findMatchingRegion((MemorySim*)pMemory, address, size);
    uint32_t      endAddress = address + size;
    Watchpoint    watchpoint = {type, address, endAddress};
    uint32_t      i;
    MatchResult   match = findMatchingOrHigherWatchpoint(pRegion, &watchpoint, &i);

    if (match != FOUND)
        return;
    memmove(&pRegion->pWatchpoints[i],
            &pRegion->pWatchpoints[i+1],
            sizeof(*pRegion->pWatchpoints) * (pRegion->watchpointCount - i - 1));
    pRegion->watchpointCount--;
}


__throws void MemorySim_SetHardwareWatchpoint(IMemory* pMemory, uint32_t address, uint32_t size, WatchpointType type)
{
    setWatchpoint(pMemory, address, size, type);
}


__throws void MemorySim_ClearHardwareWatchpoint(IMemory* pMemory, uint32_t address, uint32_t size, WatchpointType type)
{
    clearWatchpoint(pMemory, address, size, type);
}


int  MemorySim_WasWatchpointEncountered(IMemory* pMemory)
{
    MemorySim* pThis = (MemorySim*)pMemory;
    int        wasEncountered = pThis->watchpointEncountered;
    
    pThis->watchpointEncountered = 0;
    return wasEncountered;
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


static void* getDataPointer(MemorySim* pThis, uint32_t address, uint32_t size, AccessType type)
{
    MemoryRegion* pRegion = findMatchingRegion(pThis, address, size);
    uint32_t regionOffset = address - pRegion->baseAddress;
    if (type == WRITING && pRegion->readOnly)
        __throw(busErrorException);
    checkForBreakWatchPoint(pThis, pRegion, address, size, type);
    return pRegion->pData + regionOffset;
}

static void checkForBreakWatchPoint(MemorySim* pThis, 
                                    MemoryRegion* pRegion,
                                    uint32_t address, uint32_t size, AccessType type)
{
    uint32_t endAddress = address + size;
    uint32_t i;
    
    for (i = 0 ; i < pRegion->watchpointCount ; i++)
    {
        Watchpoint* pWatchpoint = &pRegion->pWatchpoints[i];
        
        if ((type & pWatchpoint->type) == 0)
            continue;
        if (pWatchpoint->type == WATCHPOINT_BREAKPOINT)
        {
            if (size == sizeof(uint16_t) && accessInRange(pWatchpoint, address, endAddress))
                __throw(hardwareBreakpointException);
        }
        else if (accessInRange(pWatchpoint, address, endAddress))
        {
            pThis->watchpointEncountered++;
        }
        else if (pWatchpoint->startAddress > address)
        {
            return;
        }
    }
}

static int accessInRange(Watchpoint* pWatchpoint, uint32_t startAddress, uint32_t endAddress)
{
    return startAddress >= pWatchpoint->startAddress && endAddress <= pWatchpoint->endAddress;
}
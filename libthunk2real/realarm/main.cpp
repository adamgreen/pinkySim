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
/* Test harness for my heading class. */
#include <mbed.h>
#include <mri.h>
#include "mpu.h"


#ifdef TEST_UNALIGNED
    static void attemptUnalignedAccess()
    {
        uint32_t words[2] = { 0x01234567, 0x89abcdef };
        uint32_t word;
    
        word = *(uint32_t*)((char*)words + 1);
        printf("word = %08lx\n", word);
    }
#else
    static void attemptUnalignedAccess()
    {
    }
#endif /* TEST_UNALIGNED */


#ifdef TEST_WRITEFAULT
    static void attemptMemoryWriteFault()
    {
        *(int*)0xFFFFFFFC = 0;
    }
#else
    static void attemptMemoryWriteFault()
    {
    }
#endif /* TEST_WRITEFAULT */


static void disallowUnalignedAccesses()
{
    SCB->CCR |= SCB_CCR_UNALIGN_TRP_Msk;
}

static void configureHighestMpuRegionToAccessAllMemoryWithNoCaching(void)
{
    static const uint32_t regionToStartAtAddress0 = 0U;
    static const uint32_t regionReadWrite = 1  << MPU_RASR_AP_SHIFT;
    static const uint32_t regionSizeAt4GB = 31 << MPU_RASR_SIZE_SHIFT; /* 4GB = 2^(31+1) */
    static const uint32_t regionEnable    = MPU_RASR_ENABLE;
    static const uint32_t regionSizeAndAttributes = regionReadWrite | regionSizeAt4GB | regionEnable;
    
    prepareToAccessMPURegion(getHighestMPUDataRegionIndex());
    setMPURegionAddress(regionToStartAtAddress0);
    setMPURegionAttributeAndSize(regionSizeAndAttributes);
}

static void disableWriteCaching()
{
    disableMPU();
    configureHighestMpuRegionToAccessAllMemoryWithNoCaching();    
    enableMPUWithHardAndNMIFaults();
}


int main() 
{
    disallowUnalignedAccesses();
    disableWriteCaching();

    attemptUnalignedAccess();
    attemptMemoryWriteFault();
    
    for (;;)
    {
        __debugbreak();
    }

    return 0;
}

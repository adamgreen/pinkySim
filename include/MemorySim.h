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
#ifndef _MEMORY_SIM_H_
#define _MEMORY_SIM_H_

#include <IMemory.h>
#include <try_catch.h>

IMemory*      MemorySim_Init(void);
void          MemorySim_Uninit(IMemory* pMemory);
__throws void MemorySim_CreateRegion(IMemory* pMemory, uint32_t baseAddress, uint32_t size);
void          MemorySim_MakeRegionReadOnly(IMemory* pMemory, uint32_t baseAddress);

#endif /* _MEMORY_SIM_H_ */

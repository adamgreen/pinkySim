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
#ifndef _SIMPLE_MEMORY_H_
#define _SIMPLE_MEMORY_H_

#include <IMemory.h>


/* Number of separate 32-bit words which can be set. */
#define SIMPLE_MEMORY_REGION_COUNT 10

/* Read/Write flags passed into SimpleMemory_SetMemory() method. */
#define READ_WRITE 0
#define READ_ONLY  1


IMemory* SimpleMemory_Init(void);

__throws void SimpleMemory_SetMemory(IMemory* pMem, uint32_t address, uint32_t value, int readOnly);


#endif /* _SIMPLE_MEMORY_H_ */

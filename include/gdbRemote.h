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
#ifndef _GDB_REMOTE_H_
#define _GDB_REMOTE_H_


#include <pinkysim.h>


void gdbRemoteGetRegisters(PinkySimContext* pContext);
void gdbRemoteSetRegisters(const PinkySimContext* pContext);
void gdbRemoteReadMemory(uint32_t address, void* pReadBuffer, uint32_t readSize);
void gdbRemoteWriteMemory(uint32_t address, const void* pWriteBuffer, uint32_t writeSize);
void gdbRemoteSingleStep(void);


#endif /* _GDB_REMOTE_H_ */

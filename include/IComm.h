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
#ifndef _ICOMM_H_
#define _ICOMM_H_


typedef struct IComm IComm;

typedef struct ICommVTable
{
    int (* shouldExit)(IComm* pThis);
} ICommVTable;

struct IComm
{
    ICommVTable* pVTable;
};


static inline int IComm_ShouldExit(IComm* pThis)
{
    return pThis->pVTable->shouldExit(pThis);
}


#endif /* _ICOMM_H_ */

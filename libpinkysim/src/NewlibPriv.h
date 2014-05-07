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
#ifndef _NEWLIB_PRIV_H_
#define _NEWLIB_PRIV_H_


int handleNewlibSemihostWriteRequest(PlatformSemihostParameters* pSemihostParameters);
int handleNewlibSemihostReadRequest(PlatformSemihostParameters* pSemihostParameters);
int handleNewlibSemihostOpenRequest(PlatformSemihostParameters* pSemihostParameters);
int handleNewlibSemihostUnlinkRequest(PlatformSemihostParameters* pSemihostParameters);
int handleNewlibSemihostLSeekRequest(PlatformSemihostParameters* pSemihostParameters);
int handleNewlibSemihostCloseRequest(PlatformSemihostParameters* pSemihostParameters);
int handleNewlibSemihostFStatRequest(PlatformSemihostParameters* pSemihostParameters);
int handleNewlibSemihostStatRequest(PlatformSemihostParameters* pSemihostParameters);
int handleNewlibSemihostRenameRequest(PlatformSemihostParameters* pSemihostParameters);


#endif /* _NEWLIB_PRIV_H_ */

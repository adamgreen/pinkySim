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
#ifndef _SEMIHOST_THUNKS_H_
#define _SEMIHOST_THUNKS_H_

#include <sys/stat.h>


int semihostWrite(int file, char *ptr, int len);
int semihostRead(int file, char *ptr, int len);
int semihostOpen(const char *pFilename, int flags, int mode);
int semihostRename(const char *pOldFilename, const char *pNewFilename);
int semihostUnlink(const char *pFilename);
int semihostStat(const char *pFilename, struct stat *pStat);
int semihostLSeek(int file, int offset, int whence);
int semihostClose(int file);
int semihostFStat(int file, struct stat *pStat);
void semihostExit(int code);


#endif /* _SEMIHOST_THUNKS_H_ */
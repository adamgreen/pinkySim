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
/* Declaration of routines that need to be provided for a specific target hardware platform before mri can be used to
   as a debug conduit for it. */
#ifndef _COMM_H_
#define _COMM_H_

#include <stdint.h>


uint32_t  commHasReceiveData(void);
int       commReceiveChar(void);
void      commSendChar(int character);
int       commCausedInterrupt(void);
void      commClearInterrupt(void);
int       commShouldWaitForGdbConnect(void);
int       commSharingWithApplication(void);
void      commPrepareToWaitForGdbConnection(void);
int       commIsWaitingForGdbToConnect(void);
void      commWaitForReceiveDataToStop(void);
int       commUartIndex(void);


#endif /* _COMM_H_ */

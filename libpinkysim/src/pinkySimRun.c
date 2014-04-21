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
#include "pinkySim.h"


int pinkySimRun(PinkySimContext* pContext, int (*callback)(PinkySimContext*))
{
    int result;
    
    do
    {
        result = callback ? callback(pContext) : PINKYSIM_STEP_OK;
        if (result == PINKYSIM_STEP_OK)
            result = pinkySimStep(pContext);
    } while (result == PINKYSIM_STEP_OK);
    return result;
}
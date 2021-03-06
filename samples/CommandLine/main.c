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
#include <stdio.h>


int main(int argc, const char** argv)
{
    int i;

    printf("Dumping %d command line arguments\n", argc);
    for (i = 0 ; i < argc ; i++)
    {
        printf("  %s\n", argv[i]);
    }

    return argc;
}

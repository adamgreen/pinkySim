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
/* Basic unit test for C Standard I/O routines. */
#include <stdio.h>


int main(void)
{
    int Value = -1;

    printf("Standard I/O Tests\r\n");

    printf("Test 1: printf() test\r\n");

    printf("Test 2: scanf() test\r\n");
    printf("    Type number and press Enter: ");
    scanf("%d", &Value);
    printf("\n    Your value was: %d\r\n", Value);

    fprintf(stdout, "Test 3: fprintf(stdout, ...) test\r\n");

    fprintf(stderr, "Test 4: fprintf(stderr, ...) test\r\n");

    printf("Test 5: fscanf(stdin, ...) test\r\n");
    printf("    Type number and press Enter: ");
    fscanf(stdin, "%d", &Value);
    printf("\n    Your value was: %d\r\n", Value);

    printf("Test complete\r\n");

    return 0;
}

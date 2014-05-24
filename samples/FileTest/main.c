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
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>


int main(void)
{
    static const char Filename[] = "foo.bar";
    unsigned char     TestBuffer[256];
    unsigned char     ReadBuffer[256];
    size_t            i;

    printf("Hello World\n");

    // Fill in test buffer with every byte value possible.
    for (i = 0 ; i < sizeof(TestBuffer) ; i++)
    {
        TestBuffer[i] = i;
    }
    memset(ReadBuffer, 0, sizeof(ReadBuffer));

    // Create a file with this data.
    FILE* fp = fopen(Filename, "w");
    if (!fp)
    {
        perror("Failed to open file for writing");
        return 1;
    }

    int BytesWritten = fwrite(TestBuffer, 1, sizeof(TestBuffer), fp);
    if (BytesWritten != sizeof(TestBuffer))
    {
        perror("Failed to write all of the bytes to test file.");
        return 1;
    }

    fclose(fp);
    fp = NULL;

    // Now reopen the file and read in the data and make sure it matches.
    fp = fopen(Filename, "r");
    if (!fp)
    {
        perror("Failed to open file for writing");
        return 1;
    }

    int BytesRead = fread(ReadBuffer, 1, sizeof(ReadBuffer), fp);
    if (BytesRead != sizeof(ReadBuffer))
    {
        perror("Failed to read all of the bytes from test file.");
        return 1;
    }
    if (0 != memcmp(TestBuffer, ReadBuffer, sizeof(TestBuffer)))
    {
        fprintf(stderr, "File data did match expected value.\n");
        return 1;
    }

    struct stat statStruct;
    int fstatResults = fstat(fileno(fp), &statStruct);
    if (statStruct.st_size != 256)
    {
        fprintf(stderr, "fstat() returned wrong file size of %ld\n", statStruct.st_size);
        return 1;
    }
    if (fstatResults != 0)
    {
        perror("Failed fstat() call.");
        return 1;
    }

    fclose(fp);
    fp = NULL;

    int statResults = stat(Filename, &statStruct);
    if (statStruct.st_size != 256)
    {
        fprintf(stderr, "stat() returned wrong file size of %ld\n", statStruct.st_size);
        return 1;
    }
    if (statResults != 0)
    {
        perror("Failed sstat() call.");
        return 1;
    }

    // Can delete the file now that we are done with it.
    remove(Filename);

    printf("\nTest completed\n");

    return 0;
}

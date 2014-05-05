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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


/* Labels exported by linker from pinkySim.ld */
extern uint32_t __StackTop[1];
extern uint32_t __bss_start__[1];
extern uint32_t __bss_end__[1];
extern uint32_t __data_start__[1];
extern uint32_t __data_end__[1];
extern uint32_t __etext[1];


/* Function Prototypes. */
void Reset_Handler(int argc, const char** argv);
void software_init_hook(void) __attribute((weak));
void hardware_init_hook(void) __attribute((weak));
void __libc_fini_array(void);
void __libc_init_array(void);
int  main(int argc, const char** argv);


/* The linker script will place isr_vector at the very beginning of the FLASH where ARMv6-M expects it. */
/* PinkySim only recognizes the first 2 ARMv6-M vectors: Initial SP value and Reset handler. */
const uint32_t isr_vector[2] = { (uint32_t)__StackTop, (uint32_t)Reset_Handler };


/* This function is the first thing executed after reset. It sets up the C runtime and then calls main(). */
void Reset_Handler(int argc, const char** argv)
{
    int dataSize = (int)__data_end__ - (int)__data_start__;
    int bssSize = (int)__bss_end__ - (int)__bss_start__;

    memcpy(__data_start__, __etext, dataSize);
    memset(__bss_start__, 0, bssSize);

    hardware_init_hook();
    software_init_hook();
    atexit(__libc_fini_array);
    __libc_init_array();
    
    /* Call user's main and then pass the returned result to exit. */
    exit( main(argc, argv) );
}

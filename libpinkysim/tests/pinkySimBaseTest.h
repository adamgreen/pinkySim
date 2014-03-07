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

// Include headers from C modules under test.
extern "C"
{
    #include "pinkySim.h"
}

// Include standard headers.
#include <assert.h>

// Include C++ headers for test harness.
#include "CppUTest/TestHarness.h"

// Register to skip in validateUnchangedRegisters.
#define SKIP(R) (1 << (R))


class pinkySimBase : public Utest
{
protected:
    int             m_stepReturn;
    PinkySimContext m_context;
    
    void setup()
    {
        m_stepReturn = -1;
        initContext();
    }

    void teardown()
    {
        CHECK_TRUE(m_context.xPSR & EPSR_T);
        CHECK_EQUAL(0, m_context.xPSR & IPSR_MASK);
    }
    
    void initContext()
    {
        memset(&m_context, 0, sizeof(m_context));
        
        /* By default we will place processor in Thumb mode. */
        m_context.xPSR = EPSR_T;
        
        /* Place 0x11111111 in R1, 0x22222222 in R2, etc. */
        uint32_t value = 0;
        assert ( 13 == sizeof(m_context.R) / sizeof(m_context.R[0]) );
        for (int i = 0 ; i < 13 ; i++)
        {
            m_context.R[i] = value;
            value += 0x11111111;
        }
    }
    
    void emitInstruction16(const char* pEncoding, ...)
    {
        uint16_t    instr = 0;
        const char* p = pEncoding + 15;
        char        curr = '\0';
        uint32_t    field = 0;
        va_list     valist;
        
        assert (16 == strlen(pEncoding));
        va_start(valist, pEncoding);
        while (p >= pEncoding)
        {
            char c = *p;
            
            instr >>= 1;
            field >>= 1;
            if (c == '1')
            {
                instr |= (1 << 15);
            }
            else if (c == '0')
            {
                instr |= (0 << 15);
            }
            else if (c != curr)
            {
                field = va_arg(valist, uint32_t);
                instr |= ((field & 1) << 15);
                curr = c;
            }
            else if (c == curr)
            {
                instr |= ((field & 1) << 15);
            }
            
            p--;
            
        }
        va_end(valist);
        m_context.memory = (uint32_t)instr;
    }
    
    void setXPSRbits(uint32_t bits)
    {
        m_context.xPSR |= bits;
    }
    
    void validateUnchangedRegisters(uint32_t registersToSkip)
    {
        uint32_t value = 0;
        for (int i = 0 ; i < 13 ; i++)
        {
            if (!(registersToSkip & 1))
            {
                CHECK_EQUAL(value, m_context.R[i]);
            }
            value += 0x11111111;
            registersToSkip >>= 1;
        }
    }
};

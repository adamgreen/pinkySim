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


class pinkySimBase : public Utest
{
protected:
    int             m_expectedStepReturn;
    int             m_expectedAPSRflags;
    uint32_t        m_expectedRegisterValues[13];
    PinkySimContext m_context;
    
    void setup()
    {
        m_expectedStepReturn = PINKYSIM_STEP_OK;
        initContext();
    }

    void teardown()
    {
    }
    
    void setExpectedStepReturn(int expectedStepReturn)
    {
        m_expectedStepReturn = expectedStepReturn;
    }
    
    void setExpectedAPSRflags(const char* pExpectedFlags)
    {
        // Remember what expected APSR flags should be after instruction execution and flip initial flag state to make
        // sure that simular correctly flips the state and doesn't just get lucky to match a pre-existing condition.
        while (*pExpectedFlags)
        {
            switch (*pExpectedFlags)
            {
            case 'n':
                m_expectedAPSRflags &= ~APSR_N;
                m_context.xPSR |= APSR_N;
                break;
            case 'N':
                m_expectedAPSRflags |= APSR_N;
                m_context.xPSR &= ~APSR_N;
                break;
            case 'z':
                m_expectedAPSRflags &= ~APSR_Z;
                m_context.xPSR |= APSR_Z;
                break;
            case 'Z':
                m_expectedAPSRflags |= APSR_Z;
                m_context.xPSR &= ~APSR_Z;
                break;
            case 'c':
                m_expectedAPSRflags &= ~APSR_C;
                m_context.xPSR |= APSR_C;
                break;
            case 'C':
                m_expectedAPSRflags |= APSR_C;
                m_context.xPSR &= ~APSR_C;
                break;
            case 'v':
                m_expectedAPSRflags &= ~APSR_V;
                m_context.xPSR |= APSR_V;
                break;
            case 'V':
                m_expectedAPSRflags |= APSR_V;
                m_context.xPSR &= ~APSR_V;
                break;
            }
            
            pExpectedFlags++;
        }
    }
    
    void setExpectedRegisterValue(int index, uint32_t expectedValue)
    {
        assert (index >= 0 && index < sizeof(m_context.R)/sizeof(m_context.R[0]));
        m_expectedRegisterValues[index] = expectedValue;
    }
    
    void initContext()
    {
        memset(&m_context, 0, sizeof(m_context));
        
        /* By default we will place processor in Thumb mode. */
        m_context.xPSR = EPSR_T;
        m_expectedAPSRflags = 0;
 
        /* Place 0x11111111 in R1, 0x22222222 in R2, etc. */
        uint32_t value = 0;
        assert ( 13 == sizeof(m_context.R) / sizeof(m_context.R[0]) );
        for (int i = 0 ; i < 13 ; i++)
        {
            m_context.R[i] = value;
            m_expectedRegisterValues[i] = value;
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
    
    void pinkySimStep(PinkySimContext* pContext)
    {
        CHECK_EQUAL(m_expectedStepReturn, ::pinkySimStep(pContext));
        validateXPSR();
        validateRegisters();
    }
    
    void validateXPSR()
    {
        CHECK_EQUAL(m_expectedAPSRflags, m_context.xPSR & APSR_NZCV);
        CHECK_TRUE(m_context.xPSR & EPSR_T);
        CHECK_EQUAL(0, m_context.xPSR & IPSR_MASK);
    }
    
    void validateRegisters()
    {
        for (int i = 0 ; i < 13 ; i++)
            CHECK_EQUAL(m_expectedRegisterValues[i], m_context.R[i]);
    }
};

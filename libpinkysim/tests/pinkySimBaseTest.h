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
    #include <pinkySim.h>
    #include <SimpleMemory.h>
}

// Include standard headers.
#include <assert.h>

// Include C++ headers for test harness.
#include "CppUTest/TestHarness.h"

// Initial values for special registers.
#define INITIAL_SP 0x10008000
#define INITIAL_LR 0xFFFFFFFF
#define INITIAL_PC 0x10000000


class pinkySimBase : public Utest
{
protected:
    int             m_expectedStepReturn;
    int             m_expectedXPSRflags;
    uint32_t        m_expectedRegisterValues[13];
    uint32_t        m_expectedSPmain;
    uint32_t        m_expectedLR;
    uint32_t        m_expectedPC;
    uint32_t        m_emitAddress;
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
    
    void setExpectedXPSRflags(const char* pExpectedFlags)
    {
        // Remember what expected APSR flags should be after instruction execution and flip initial flag state to make
        // sure that simular correctly flips the state and doesn't just get lucky to match a pre-existing condition.
        while (*pExpectedFlags)
        {
            switch (*pExpectedFlags)
            {
            case 'n':
                m_expectedXPSRflags &= ~APSR_N;
                m_context.xPSR |= APSR_N;
                break;
            case 'N':
                m_expectedXPSRflags |= APSR_N;
                m_context.xPSR &= ~APSR_N;
                break;
            case 'z':
                m_expectedXPSRflags &= ~APSR_Z;
                m_context.xPSR |= APSR_Z;
                break;
            case 'Z':
                m_expectedXPSRflags |= APSR_Z;
                m_context.xPSR &= ~APSR_Z;
                break;
            case 'c':
                m_expectedXPSRflags &= ~APSR_C;
                m_context.xPSR |= APSR_C;
                break;
            case 'C':
                m_expectedXPSRflags |= APSR_C;
                m_context.xPSR &= ~APSR_C;
                break;
            case 'v':
                m_expectedXPSRflags &= ~APSR_V;
                m_context.xPSR |= APSR_V;
                break;
            case 'V':
                m_expectedXPSRflags |= APSR_V;
                m_context.xPSR &= ~APSR_V;
                break;
            case 't':
                m_expectedXPSRflags &= ~EPSR_T;
                break;
            case 'T':
                m_expectedXPSRflags |= EPSR_T;
                break;
            }
            
            pExpectedFlags++;
        }
    }
    
    void setExpectedRegisterValue(int index, uint32_t expectedValue)
    {
        assert (index >= 0 && index <= PC);

        if (index == PC)
            m_expectedPC = expectedValue;
        else if (index == LR)
            m_expectedLR = expectedValue;
        else if (index == SP)
            m_expectedSPmain = expectedValue;
        else
            m_expectedRegisterValues[index] = expectedValue;
    }
    
    void setRegisterValue(int index, uint32_t value)
    {
        assert (index >= 0 && index <= PC);

        setExpectedRegisterValue(index, value);
        if (index == PC)
        {
            setExpectedRegisterValue(index, value + 2);
            m_context.pc = value;
        }
        else if (index == LR)
        {
            m_context.lr = value;
        }
        else if (index == SP)
        {
            m_context.spMain = value;
        }
        else
        {
            m_context.R[index] = value;
        }
    }
    
    void initContext()
    {
        memset(&m_context, 0, sizeof(m_context));
        
        m_context.pMemory = SimpleMemory_Init();
        SimpleMemory_SetMemory(m_context.pMemory, INITIAL_PC, 0x0, READ_WRITE);

        /* By default we will place the processor in Thumb mode. */
        m_context.xPSR = EPSR_T;
        m_expectedXPSRflags |= EPSR_T;
        
        /* Randomly initialize each APSR flag to help verify that the simulator doesn't clear/set a bit that the 
           specification indicates shouldn't be modified by an instruction. */
        for (uint32_t bit = APSR_N ; bit >= APSR_V ; bit >>= 1)
        {
            int setOrClear = rand() & 1;
            if (setOrClear)
            {
                m_context.xPSR |= bit;
                m_expectedXPSRflags |= bit;
            }
            else
            {
                m_context.xPSR &= ~bit;
                m_expectedXPSRflags &= ~bit;
            }
        }
 
        /* Place 0x11111111 in R1, 0x22222222 in R2, etc. */
        uint32_t value = 0;
        assert ( 13 == sizeof(m_context.R) / sizeof(m_context.R[0]) );
        for (int i = 0 ; i < 13 ; i++)
        {
            m_context.R[i] = value;
            m_expectedRegisterValues[i] = value;
            value += 0x11111111;
        }

        /* These defaults are values that would work on the LPC1768. */
        setRegisterValue(SP, INITIAL_SP);
        setRegisterValue(LR, INITIAL_LR);
        setRegisterValue(PC, INITIAL_PC);
        
        m_emitAddress = INITIAL_PC;
    }
    
    void emitInstruction16(const char* pEncoding, ...)
    {
        uint16_t    instr = 0;
        size_t      i = 0;
        char        last = '\0';
        const char* p;
        va_list     valist;
        struct Field
        {
            uint32_t value;
            char     c;
        } fields[6];
        
        assert (16 == strlen(pEncoding));
        memset(fields, 0, sizeof(fields));
        va_start(valist, pEncoding);

        // Go through pEncoding from left to right and find all fields to be inserted.
        p = pEncoding;
        while (*p)
        {
            char c = *p++;
            
            if (c != '1' && c != '0' && c != last)
            {
                // Determine if we already saw this field earlier.
                bool found = false;
                for (size_t j = 0 ; j < i ; j++)
                {
                    if (fields[j].c == c)
                        found = true;
                }
                
                // If this is the first time we have seen the field, then save its value in fields array.
                if (!found)
                {
                    assert (i < sizeof(fields)/sizeof(fields[0]));
                    
                    fields[i].value = va_arg(valist, uint32_t);
                    fields[i].c = c;
                    last = c;
                    i++;
                }
            }
        }
        va_end(valist);
        
        // Go through pEncoding again from right to left and insert field bits.
        p = pEncoding + 15;
        while (p >= pEncoding)
        {
            char c = *p--;
            
            instr >>= 1;
            
            if (c == '1')
            {
                instr |= (1 << 15);
            }
            else if (c == '0')
            {
                instr |= (0 << 15);
            }
            else
            {
                size_t j;
                for (j = 0 ; j < i ; j++)
                {
                    if (fields[j].c == c)
                        break;
                }
                assert (j != i);
                
                instr |= (fields[j].value & 1) << 15;
                fields[j].value >>= 1;
            }
        }
        
        IMemory_Write16(m_context.pMemory, m_emitAddress, instr);
        m_emitAddress += 2;
    }
    
    void pinkySimStep(PinkySimContext* pContext)
    {
        int result = ::pinkySimStep(pContext);
        CHECK_EQUAL(m_expectedStepReturn, result);
        validateXPSR();
        validateRegisters();
    }
    
    void validateXPSR()
    {
        CHECK_EQUAL(m_expectedXPSRflags, m_context.xPSR & (APSR_NZCV | EPSR_T));
        CHECK_EQUAL(0, m_context.xPSR & IPSR_MASK);
    }
    
    void validateRegisters()
    {
        for (int i = 0 ; i < 13 ; i++)
            CHECK_EQUAL(m_expectedRegisterValues[i], m_context.R[i]);
        CHECK_EQUAL(m_expectedSPmain, m_context.spMain);
        CHECK_EQUAL(m_expectedLR, m_context.lr);
        CHECK_EQUAL(m_expectedPC, m_context.pc);
    }
    
    void setCarry()
    {
        m_context.xPSR |= APSR_C;
    }
    
    void clearCarry()
    {
        m_context.xPSR &= ~APSR_C;
    }
};

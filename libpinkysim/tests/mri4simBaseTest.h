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
    #include <common.h>
    #include <MemorySim.h>
    #include <mri4sim.h>
    #include <pinkySim.h>
    #include <printfSpy.h>
}

// Include standard headers.
#include <assert.h>

// Include C++ headers for test harness.
#include "CppUTest/TestHarness.h"
#include <mockIComm.h>

// Initial values for special registers.
#define INITIAL_SP 0x10008000
#define INITIAL_LR 0x00000000
#define INITIAL_PC 0x10004000

class mri4simBase : public Utest
{
protected:
    IMemory*         m_pMemory;
    PinkySimContext* m_pContext;
    uint32_t         m_emitAddress;
    char             m_buffer[2048];
    char*            m_pBufferCurr;
    
    void setup()
    {
        /* Most of testing will happen in RAM but set first two words of FLASH with initial SP and reset vector. */
        static const uint32_t flashImage[] = { INITIAL_SP, INITIAL_PC | 1 };
        m_pMemory = MemorySim_Init();
        MemorySim_CreateRegionsFromFlashImage(m_pMemory, flashImage, sizeof(flashImage));
        mri4simInit(m_pMemory);
        m_pContext = mri4simGetContext();
        
        /* Setup to buffer a maximum of 1024 characters sent by MRI. */
        mockIComm_InitTransmitDataBuffer(1024);
        
        /* Setup to buffer a maxmimum of 256 characters used with printf() calls. */
        printfSpy_Hook(256);

        /* Most of the times a test will want to only run the mri4simRun() loop once so set stop flag to 1. */
        mockIComm_SetShouldStopRunFlag(1);
        m_emitAddress = INITIAL_PC;
        
        resetExpectedBuffer();
    }
    
    void resetExpectedBuffer()
    {
        m_pBufferCurr = m_buffer;
    }

    void teardown()
    {
        printfSpy_Unhook();
        MemorySim_Uninit(m_pMemory);
        mockIComm_Uninit();
    }
    
    void setXPSRflags(const char* pFlags)
    {
        while (*pFlags)
        {
            switch (*pFlags)
            {
            case 'n':
                m_pContext->xPSR &= ~APSR_N;
                break;
            case 'N':
                m_pContext->xPSR |= APSR_N;
                break;
            case 'z':
                m_pContext->xPSR &= ~APSR_Z;
                break;
            case 'Z':
                m_pContext->xPSR |= APSR_Z;
                break;
            case 'c':
                m_pContext->xPSR &= ~APSR_C;
                break;
            case 'C':
                m_pContext->xPSR |= APSR_C;
                break;
            case 'v':
                m_pContext->xPSR &= ~APSR_V;
                break;
            case 'V':
                m_pContext->xPSR |= APSR_V;
                break;
            case 't':
                m_pContext->xPSR &= ~EPSR_T;
                break;
            case 'T':
                m_pContext->xPSR |= EPSR_T;
                break;
            }
            
            pFlags++;
        }
    }
    
    void setRegisterValue(int index, uint32_t value)
    {
        assert (index >= 0 && index <= PC);

        if (index == PC)
        {
            m_pContext->pc = value;
        }
        else if (index == LR)
        {
            m_pContext->lr = value;
        }
        else if (index == SP)
        {
            m_pContext->spMain = value;
        }
        else
        {
            m_pContext->R[index] = value;
        }
    }
    
    void emitBKPT(uint32_t immediate)
    {
        emitInstruction16("10111110iiiiiiii", immediate);
    }
    
    void emitSVC(uint32_t immediate)
    {
        emitInstruction16("11011111iiiiiiii", immediate);
    }
    
    void emitNOP()
    {
        emitInstruction16("1011111100000000");
    }
    
    void emitMOVimmediate(uint32_t Rd, uint32_t immediate)
    {
        emitInstruction16("00100dddiiiiiiii", Rd, immediate);
    }
    
    void emitLDRImmediate(uint32_t Rt, uint32_t Rn, uint32_t immediate)
    {
        emitInstruction16("01101iiiiinnnttt", immediate, Rn, Rt);
    }
    
    void emitLDRBImmediate(uint32_t Rt, uint32_t Rn, uint32_t immediate)
    {
        emitInstruction16("01111iiiiinnnttt", immediate, Rn, Rt);
    }

    void emitSTRImmediate(uint32_t Rt, uint32_t Rn, uint32_t immediate)
    {
        emitInstruction16("01100iiiiinnnttt", immediate, Rn, Rt);
    }

    void emitSTRHImmediate(uint32_t Rt, uint32_t Rn, uint32_t immediate)
    {
        emitInstruction16("10000iiiiinnnttt", immediate, Rn, Rt);
    }
    
    void emitUND(uint32_t immediate)
    {
        emitInstruction16("11011110iiiiiiii", immediate);
    }
    
    void emitYIELD()
    {
        emitInstruction16("1011111100010000");
    }
    
    void emitInstruction16(const char* pEncoding, ...)
    {
        va_list     valist;

        va_start(valist, pEncoding);
        emitInstruction16Varg(pEncoding, valist);
        va_end(valist);
    }
    
    void emitInstruction32(const char* pEncoding1, const char* pEncoding2, ...)
    {
        va_list     valist;

        va_start(valist, pEncoding2);
        emitInstruction16Varg(pEncoding1, valist);
        emitInstruction16Varg(pEncoding2, valist);
        va_end(valist);
    }
    
    void emitInstruction16Varg(const char* pEncoding, va_list valist)
    {
        uint16_t    instr = 0;
        size_t      i = 0;
        char        last = '\0';
        const char* p;
        struct Field
        {
            uint32_t value;
            char     c;
        } fields[6];
        
        assert (16 == strlen(pEncoding));
        memset(fields, 0, sizeof(fields));

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
        
        IMemory_Write16(m_pMemory, m_emitAddress, instr);
        m_emitAddress += 2;
    }
    
    void setCarry()
    {
        m_pContext->xPSR |= APSR_C;
    }
    
    void clearCarry()
    {
        m_pContext->xPSR &= ~APSR_C;
    }
    
    void setZero()
    {
        m_pContext->xPSR |= APSR_Z;
    }
    
    void clearZero()
    {
        m_pContext->xPSR &= ~APSR_Z;
    }
    
    void setNegative()
    {
        m_pContext->xPSR |= APSR_N;
    }
    
    void clearNegative()
    {
        m_pContext->xPSR &= ~APSR_N;
    }
    
    void setOverflow()
    {
        m_pContext->xPSR |= APSR_V;
    }
    
    void clearOverflow()
    {
        m_pContext->xPSR &= ~APSR_V;
    }
    
    void setIPSR(uint32_t ipsr)
    {
        m_pContext->xPSR = (m_pContext->xPSR & ~IPSR_MASK) | (ipsr & IPSR_MASK);
    }

    uint32_t byteSwap(uint32_t value)
    {
        return (value >> 24) | ((value >> 8) & 0xFF00) | ((value << 8) & 0xFF0000) | (value << 24);
    }
    
    void appendExpectedTPacket(uint32_t expectedSignal,
                               uint32_t expectedR12,
                               uint32_t expectedSP,
                               uint32_t expectedLR,
                               uint32_t expectedPC)
    {
        int   bytesLeft = bufferBytesLeft();
        
        int result = snprintf(m_pBufferCurr, bytesLeft,
                             "$T%02x0c:%08x;0d:%08x;0e:%08x;0f:%08x;#",
                             expectedSignal,
                             byteSwap(expectedR12),
                             byteSwap(expectedSP),
                             byteSwap(expectedLR),
                             byteSwap(expectedPC));
        assert(result < bytesLeft);
        m_pBufferCurr += result;
    }
    
    int bufferBytesLeft()
    {
        return (m_buffer + sizeof(m_buffer)) - m_pBufferCurr;
    }

    void appendExpectedString(const char* pExpectedString)
    {
        int   bytesLeft = bufferBytesLeft();
        
        int result = snprintf(m_pBufferCurr, bytesLeft, "%s", pExpectedString);
        assert(result < bytesLeft);
        m_pBufferCurr += result;
    }
    
    void appendExpectedOPacket(const char* pExpectedString)
    {
        appendExpectedString("$O");
        int bytesLeft = bufferBytesLeft();
        while (*pExpectedString)
        {
            int result = snprintf(m_pBufferCurr, bytesLeft, "%02x", *pExpectedString);
            assert (result < bytesLeft);
            m_pBufferCurr += result;
            bytesLeft -= result;
            pExpectedString++;
        }
        appendExpectedString("#");
    }
    
    const char* checksumExpected()
    {
        return mockIComm_ChecksumData(m_buffer);
    }
};

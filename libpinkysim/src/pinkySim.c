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
#include <assert.h>
#include "pinkySim.h"

/* Boolean values */
#define FALSE 0
#define TRUE  1

/* Types of shift/rotate operations. */
typedef enum SRType
{
    SRType_LSL,
    SRType_LSR,
    SRType_ASR,
    SRType_RRX,
    SRType_ROR
} SRType;

/* Decoded shift/rotate operation. */
typedef struct DecodedImmShift
{
    SRType   type;
    uint32_t n;
} DecodedImmShift;

/* Shift operation results. */
typedef struct ShiftResults
{
    uint32_t result;
    uint32_t carryOut;
} ShiftResults;

/* Results from addition/subtraction. */
typedef struct AddResults
{
    uint32_t result;
    int      carryOut;
    int      overflow;
} AddResults;

/* Function Prototypes */
static int shiftAddSubtractMoveCompare(PinkySimContext* pContext, uint16_t instr);
static int lslImmediate(PinkySimContext* pContext, uint16_t instr);
static int ConditionPassedForNonBranchInstr(const PinkySimContext* pContext);
static int InITBlock(const PinkySimContext* pContext);
static DecodedImmShift DecodeImmShift(uint32_t typeBits, uint32_t imm5);
static ShiftResults Shift_C(uint32_t value, SRType type, uint32_t amount, uint32_t carry_in);
static ShiftResults LSL_C(uint32_t x, uint32_t shift);
static ShiftResults LSR_C(uint32_t x, uint32_t shift);
static ShiftResults ASR_C(uint32_t x, uint32_t shift);
static uint32_t getReg(const PinkySimContext* pContext, uint32_t reg);
static void setReg(PinkySimContext* pContext, uint32_t reg, uint32_t value);
static int lsrImmediate(PinkySimContext* pContext, uint16_t instr);
static int asrImmediate(PinkySimContext* pContext, uint16_t instr);
static int addRegisterT1(PinkySimContext* pContext, uint16_t instr);
static uint32_t Shift(uint32_t value, SRType type, uint32_t amount, uint32_t carryIn);
static AddResults AddWithCarry(uint32_t x, uint32_t y, uint32_t carryInAsBit);
static int subRegister(PinkySimContext* pContext, uint16_t instr);
static int addImmediateT1(PinkySimContext* pContext, uint16_t instr);
static int subImmediateT1(PinkySimContext* pContext, uint16_t instr);
static int movImmediate(PinkySimContext* pContext, uint16_t instr);
static int cmpImmediate(PinkySimContext* pContext, uint16_t instr);
static int addImmediateT2(PinkySimContext* pContext, uint16_t instr);
static int subImmediateT2(PinkySimContext* pContext, uint16_t instr);


int pinkySimStep(PinkySimContext* pContext)
{
    uint16_t instr = (uint16_t)pContext->memory;
    pContext->instr1 = instr;
    if ((instr & 0xC000) == 0x0000)
        return shiftAddSubtractMoveCompare(pContext, instr);

    return PINKYSIM_STEP_UNDEFINED;
}

static int shiftAddSubtractMoveCompare(PinkySimContext* pContext, uint16_t instr)
{
    if ((instr & 0x3800) == 0x0000)
        return lslImmediate(pContext, instr);
    else if ((instr & 0x3800) == 0x0800)
        return lsrImmediate(pContext, instr);
    else if ((instr & 0x3800) == 0x1000)
        return asrImmediate(pContext, instr);
    else if ((instr & 0x3E00) == 0x1800)
        return addRegisterT1(pContext, instr);
    else if ((instr & 0x3E00) == 0x1A00)
        return subRegister(pContext, instr);
    else if ((instr & 0x3E00) == 0x1C00)
        return addImmediateT1(pContext, instr);
    else if ((instr & 0x3E00) == 0x1E00)
        return subImmediateT1(pContext, instr);
    else if ((instr & 0x3800) == 0x2000)
        return movImmediate(pContext, instr);
    else if ((instr & 0x3800) == 0x2800)
        return cmpImmediate(pContext, instr);
    else if ((instr & 0x3800) == 0x3000)
        return addImmediateT2(pContext, instr);
    else if ((instr & 0x3800) == 0x3800)
        return subImmediateT2(pContext, instr);
    else
        return PINKYSIM_STEP_UNDEFINED;
}

static int lslImmediate(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t        imm5 = (instr & (0x1F << 6)) >> 6;
        uint32_t        d = instr & 0x7;
        uint32_t        m = (instr & (0x7 << 3)) >> 3;
        int             setFlags = !InITBlock(pContext);
        DecodedImmShift decodedShift = DecodeImmShift(0x0, imm5);
        ShiftResults    shiftResults;

        shiftResults = Shift_C(getReg(pContext, m), SRType_LSL, decodedShift.n, pContext->xPSR & APSR_C);
        setReg(pContext, d, shiftResults.result);
        if (setFlags)
        {
            pContext->xPSR &= ~APSR_NZC;
            if (shiftResults.result & (1 << 31))
                pContext->xPSR |= APSR_N;
            if (shiftResults.result == 0)
                pContext->xPSR |= APSR_Z;
            if (shiftResults.carryOut)
                pContext->xPSR |= APSR_C;
        }
    }

    return PINKYSIM_STEP_OK;
}

static int ConditionPassedForNonBranchInstr(const PinkySimContext* pContext)
{
    /* In ARMv6-M, only branches can be conditional so always pass. */
    return TRUE;
}

static int InITBlock(const PinkySimContext* pContext)
{
    /* In ARMv6-M, the processor is never in an IT, If-Then, block. */
    return FALSE;
}

static DecodedImmShift DecodeImmShift(uint32_t typeBits, uint32_t imm5)
{
    /* UNDONE: Needs to do much more as tests progress. */
    DecodedImmShift results;

    assert ((typeBits & 0x3) == typeBits);
    switch (typeBits)
    {
    case 0x0:
        results.type = SRType_LSL;
        results.n = imm5;
        break;
    case 0x1:
        results.type = SRType_LSR;
        results.n = (imm5 == 0) ? 32 : imm5;
        break;
    case 0x2:
        results.type = SRType_ASR;
        results.n = (imm5 == 0) ? 32 : imm5;
        break;
    default:
        assert (FALSE);
    }
    
    return results;
}

static ShiftResults Shift_C(uint32_t value, SRType type, uint32_t amount, uint32_t carry_in)
{
    ShiftResults results;

    assert (type != SRType_RRX || amount == 1);
    
    if (amount == 0)
    {
        results.result = value;
        results.carryOut = 0;
    }
    else
    {
        switch (type)
        {
        case SRType_LSL:
            results = LSL_C(value, amount);
            break;
        case SRType_LSR:
            results = LSR_C(value, amount);
            break;
        case SRType_ASR:
            results = ASR_C(value, amount);
            break;
        case SRType_RRX:
        case SRType_ROR:
            // UNDONE: Actually implement these modes as tests progress.
            assert ( FALSE );
        }
    }
    return results;
}

static ShiftResults LSL_C(uint32_t x, uint32_t shift)
{
    ShiftResults results;
    
    assert (shift < 32);

    results.carryOut = (x & (1 << (32 - shift)));
    results.result = x << shift;
    return results;
}

static ShiftResults LSR_C(uint32_t x, uint32_t shift)
{
    ShiftResults results;
    
    assert (shift > 0 && shift <= 32);

    results.carryOut = (x & (1 << (shift - 1)));
    results.result = (shift == 32) ? 0 : x >> shift;
    return results;
}

static ShiftResults ASR_C(uint32_t x, uint32_t shift)
{
    ShiftResults results;
    
    assert (shift > 0 && shift <= 32);

    results.carryOut = (x & (1 << (shift - 1)));
    if (shift == 32)
    {
        if (x & 0x80000000)
            results.result = 0xFFFFFFFF;
        else
            results.result = 0x00000000;
    }
    else
    {
        results.result = (uint32_t)((int32_t)x >> shift);
    }
    return results;
}

static uint32_t getReg(const PinkySimContext* pContext, uint32_t reg)
{
    assert (reg < sizeof(pContext->R)/sizeof(pContext->R[0]));
    
    return pContext->R[reg];
}

static void setReg(PinkySimContext* pContext, uint32_t reg, uint32_t value)
{
    assert (reg < sizeof(pContext->R)/sizeof(pContext->R[0]));

    pContext->R[reg] = value;
}

static int lsrImmediate(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t        imm5 = (instr & (0x1F << 6)) >> 6;
        uint32_t        d = instr & 0x7;
        uint32_t        m = (instr & (0x7 << 3)) >> 3;
        int             setFlags = !InITBlock(pContext);
        DecodedImmShift decodedShift = DecodeImmShift(0x1, imm5);
        ShiftResults    shiftResults;

        shiftResults = Shift_C(getReg(pContext, m), SRType_LSR, decodedShift.n, pContext->xPSR & APSR_C);
        setReg(pContext, d, shiftResults.result);
        if (setFlags)
        {
            pContext->xPSR &= ~APSR_NZC;
            // UNDONE: The N flag is always cleared from a logical right shift.  Improve coverage by removing code.
            //if (shiftResults.result & (1 << 31))
            //    pContext->xPSR |= APSR_N;
            if (shiftResults.result == 0)
                pContext->xPSR |= APSR_Z;
            if (shiftResults.carryOut)
                pContext->xPSR |= APSR_C;
        }
    }

    return PINKYSIM_STEP_OK;
}

static int asrImmediate(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t        imm5 = (instr & (0x1F << 6)) >> 6;
        uint32_t        d = instr & 0x7;
        uint32_t        m = (instr & (0x7 << 3)) >> 3;
        int             setFlags = !InITBlock(pContext);
        DecodedImmShift decodedShift = DecodeImmShift(0x2, imm5);
        ShiftResults    shiftResults;

        shiftResults = Shift_C(getReg(pContext, m), SRType_ASR, decodedShift.n, pContext->xPSR & APSR_C);
        setReg(pContext, d, shiftResults.result);
        if (setFlags)
        {
            pContext->xPSR &= ~APSR_NZC;
            if (shiftResults.result & (1 << 31))
                pContext->xPSR |= APSR_N;
            if (shiftResults.result == 0)
                pContext->xPSR |= APSR_Z;
            if (shiftResults.carryOut)
                pContext->xPSR |= APSR_C;
        }
    }

    return PINKYSIM_STEP_OK;
}

static int addRegisterT1(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t        d = instr & 0x7;
        uint32_t        n = (instr & (0x7 << 3)) >> 3;
        uint32_t        m = (instr & (0x7 << 6)) >> 6;
        int             setFlags = !InITBlock(pContext);
        DecodedImmShift decodedShift = {SRType_LSL, 0};
        uint32_t        shifted;
        AddResults      addResults;
        
        // UNDONE: Only Thumb2 instructions require this shifted value.
        shifted = Shift(getReg(pContext, m), decodedShift.type, decodedShift.n, pContext->xPSR & APSR_C);
        addResults = AddWithCarry(getReg(pContext, n), shifted, 0);

        // UNDONE: Only the T2 encoding can modify R15 (PC).
#ifdef UNDONE
        if (d == 15)
        {
            ALUWritePC(pContext, addResults.result);
        }
        else
#endif // UNDONE
        {
            setReg(pContext, d, addResults.result);

            if (setFlags)
            {
                pContext->xPSR &= ~APSR_NZCV;
                if (addResults.result & (1 << 31))
                    pContext->xPSR |= APSR_N;
                if (addResults.result == 0)
                    pContext->xPSR |= APSR_Z;
                if (addResults.carryOut)
                    pContext->xPSR |= APSR_C;
                if (addResults.overflow)
                    pContext->xPSR |= APSR_V;
            }
        }
    }

    return PINKYSIM_STEP_OK;
}

static uint32_t Shift(uint32_t value, SRType type, uint32_t amount, uint32_t carryIn)
{
    ShiftResults results = Shift_C(value, type, amount, carryIn);
    return results.result;
}

static AddResults AddWithCarry(uint32_t x, uint32_t y, uint32_t carryInAsBit)
{
    int        carryIn = carryInAsBit ? 1 : 0;
    uint64_t   unsignedSum = (uint64_t)x + (uint64_t)y + (uint64_t)carryIn;
    int64_t    signedSum = (int64_t)(int32_t)x + (int64_t)(int32_t)y + (int64_t)carryIn;
    uint32_t   result = (uint32_t)unsignedSum;
    AddResults results;

    results.result = result;
    results.carryOut = ((uint64_t)result == unsignedSum) ? 0 : 1;
    results.overflow = ((int64_t)(int32_t)result == signedSum) ? 0 : 1;
    return results;
}

static int subRegister(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t        d = instr & 0x7;
        uint32_t        n = (instr & (0x7 << 3)) >> 3;
        uint32_t        m = (instr & (0x7 << 6)) >> 6;
        int             setFlags = !InITBlock(pContext);
        DecodedImmShift decodedShift = {SRType_LSL, 0};
        uint32_t        shifted;
        AddResults      addResults;
        
        // UNDONE: Only Thumb2 instructions require this shifted value.
        shifted = Shift(getReg(pContext, m), decodedShift.type, decodedShift.n, pContext->xPSR & APSR_C);
        addResults = AddWithCarry(getReg(pContext, n), ~shifted, 1);

        setReg(pContext, d, addResults.result);

        if (setFlags)
        {
            pContext->xPSR &= ~APSR_NZCV;
            if (addResults.result & (1 << 31))
                pContext->xPSR |= APSR_N;
            if (addResults.result == 0)
                pContext->xPSR |= APSR_Z;
            if (addResults.carryOut)
                pContext->xPSR |= APSR_C;
            if (addResults.overflow)
                pContext->xPSR |= APSR_V;
        }
    }

    return PINKYSIM_STEP_OK;
}

static int addImmediateT1(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t        d = instr & 0x7;
        uint32_t        n = (instr & (0x7 << 3)) >> 3;
        uint32_t        imm32 = (instr & (0x7 << 6)) >> 6;
        int             setFlags = !InITBlock(pContext);
        AddResults      addResults;
        
        addResults = AddWithCarry(getReg(pContext, n), imm32, 0);
        setReg(pContext, d, addResults.result);
        if (setFlags)
        {
            pContext->xPSR &= ~APSR_NZCV;
            if (addResults.result & (1 << 31))
                pContext->xPSR |= APSR_N;
            if (addResults.result == 0)
                pContext->xPSR |= APSR_Z;
            if (addResults.carryOut)
                pContext->xPSR |= APSR_C;
            if (addResults.overflow)
                pContext->xPSR |= APSR_V;
        }
    }

    return PINKYSIM_STEP_OK;
}

static int subImmediateT1(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t        d = instr & 0x7;
        uint32_t        n = (instr & (0x7 << 3)) >> 3;
        uint32_t        imm32 = (instr & (0x7 << 6)) >> 6;
        int             setFlags = !InITBlock(pContext);
        AddResults      addResults;
        
        addResults = AddWithCarry(getReg(pContext, n), ~imm32, 1);
        setReg(pContext, d, addResults.result);
        if (setFlags)
        {
            pContext->xPSR &= ~APSR_NZCV;
            if (addResults.result & (1 << 31))
                pContext->xPSR |= APSR_N;
            if (addResults.result == 0)
                pContext->xPSR |= APSR_Z;
            if (addResults.carryOut)
                pContext->xPSR |= APSR_C;
            if (addResults.overflow)
                pContext->xPSR |= APSR_V;
        }
    }

    return PINKYSIM_STEP_OK;
}

static int movImmediate(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t        d = (instr & (0x7 << 8)) >> 8;
        int             setFlags = !InITBlock(pContext);
        uint32_t        imm32 = instr & 0xFF;
        uint32_t        carry = pContext->xPSR & APSR_C;
        uint32_t        result = imm32;

        setReg(pContext, d, result);
        if (setFlags)
        {
            pContext->xPSR &= ~APSR_NZC;
            
            // UNDONE: Can't move negative values with MOV immedate (only immediates 0 - 255).
            //if (result & (1 << 31))
            //    pContext->xPSR |= APSR_N;
            if (result == 0)
                pContext->xPSR |= APSR_Z;
            if (carry)
                pContext->xPSR |= APSR_C;
        }
    }

    return PINKYSIM_STEP_OK;
}

static int cmpImmediate(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t        n = (instr & (0x7 << 8)) >> 8;
        uint32_t        imm32 = instr & 0xFF;
        AddResults      addResults;
        
        addResults = AddWithCarry(getReg(pContext, n), ~imm32, 1);
        pContext->xPSR &= ~APSR_NZCV;
        if (addResults.result & (1 << 31))
            pContext->xPSR |= APSR_N;
        if (addResults.result == 0)
            pContext->xPSR |= APSR_Z;
        if (addResults.carryOut)
            pContext->xPSR |= APSR_C;
        if (addResults.overflow)
            pContext->xPSR |= APSR_V;
    }

    return PINKYSIM_STEP_OK;
}

static int addImmediateT2(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t        d = (instr & (0x7 << 8)) >> 8;
        uint32_t        n = d;
        int             setFlags = !InITBlock(pContext);
        uint32_t        imm32 = instr & 0xFF;
        AddResults      addResults;
        
        addResults = AddWithCarry(getReg(pContext, n), imm32, 0);
        setReg(pContext, d, addResults.result);
        if (setFlags)
        {
            pContext->xPSR &= ~APSR_NZCV;
            if (addResults.result & (1 << 31))
                pContext->xPSR |= APSR_N;
            if (addResults.result == 0)
                pContext->xPSR |= APSR_Z;
            if (addResults.carryOut)
                pContext->xPSR |= APSR_C;
            if (addResults.overflow)
                pContext->xPSR |= APSR_V;
        }
    }

    return PINKYSIM_STEP_OK;
}

static int subImmediateT2(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t        d = (instr & (0x7 << 8)) >> 8;
        uint32_t        n = d;
        int             setFlags = !InITBlock(pContext);
        uint32_t        imm32 = instr & 0xFF;
        AddResults      addResults;
        
        addResults = AddWithCarry(getReg(pContext, n), ~imm32, 1);
        setReg(pContext, d, addResults.result);
        if (setFlags)
        {
            pContext->xPSR &= ~APSR_NZCV;
            if (addResults.result & (1 << 31))
                pContext->xPSR |= APSR_N;
            if (addResults.result == 0)
                pContext->xPSR |= APSR_Z;
            if (addResults.carryOut)
                pContext->xPSR |= APSR_C;
            if (addResults.overflow)
                pContext->xPSR |= APSR_V;
        }
    }

    return PINKYSIM_STEP_OK;
}

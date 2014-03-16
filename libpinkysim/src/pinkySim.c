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
static ShiftResults Shift_C(uint32_t value, SRType type, uint32_t amount, uint32_t carryIn);
static ShiftResults LSL_C(uint32_t x, uint32_t shift);
static ShiftResults LSR_C(uint32_t x, uint32_t shift);
static ShiftResults ASR_C(uint32_t x, uint32_t shift);
static ShiftResults ROR_C(uint32_t x, uint32_t shift);
static uint32_t LSR(uint32_t x, uint32_t shift);
static uint32_t LSL(uint32_t x, uint32_t shift);
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
static int dataProcessing(PinkySimContext* pContext, uint16_t instr);
static int andRegister(PinkySimContext* pContext, uint16_t instr);
static int eorRegister(PinkySimContext* pContext, uint16_t instr);
static int lslRegister(PinkySimContext* pContext, uint16_t instr);
static int lsrRegister(PinkySimContext* pContext, uint16_t instr);
static int asrRegister(PinkySimContext* pContext, uint16_t instr);
static int adcRegister(PinkySimContext* pContext, uint16_t instr);
static int sbcRegister(PinkySimContext* pContext, uint16_t instr);
static int rorRegister(PinkySimContext* pContext, uint16_t instr);
static int tstRegister(PinkySimContext* pContext, uint16_t instr);
static int rsbRegister(PinkySimContext* pContext, uint16_t instr);
static int cmpRegisterT1(PinkySimContext* pContext, uint16_t instr);
static int cmnRegister(PinkySimContext* pContext, uint16_t instr);
static int orrRegister(PinkySimContext* pContext, uint16_t instr);
static int mulRegister(PinkySimContext* pContext, uint16_t instr);
static int bicRegister(PinkySimContext* pContext, uint16_t instr);
static int mvnRegister(PinkySimContext* pContext, uint16_t instr);
static int specialDataAndBranchExchange(PinkySimContext* pContext, uint16_t instr);
static int addRegisterT2(PinkySimContext* pContext, uint16_t instr);
static void ALUWritePC(PinkySimContext* pContext, uint32_t address);
static void BranchWritePC(PinkySimContext* pContext, uint32_t address);
static void BranchTo(PinkySimContext* pContext, uint32_t address);
static int cmpRegisterT2(PinkySimContext* pContext, uint16_t instr);
static int movRegister(PinkySimContext* pContext, uint16_t instr);
static int bx(PinkySimContext* pContext, uint16_t instr);
static void BXWritePC(PinkySimContext* pContext, uint32_t address);
static int blx(PinkySimContext* pContext, uint16_t instr);
static void BLXWritePC(PinkySimContext* pContext, uint32_t address);


int pinkySimStep(PinkySimContext* pContext)
{
    int      result = PINKYSIM_STEP_UNDEFINED;
    uint16_t instr = (uint16_t)pContext->memory;

    if (!(pContext->xPSR & EPSR_T))
        return PINKYSIM_STEP_HARDFAULT;
    
    // UNDONE: This is specific to 16-bit instructions.
    pContext->newPC = pContext->pc + 2;
    if ((instr & 0xC000) == 0x0000)
        result = shiftAddSubtractMoveCompare(pContext, instr);
    else if ((instr & 0xFC00) == 0x4000)
        result = dataProcessing(pContext, instr);
    else if ((instr & 0xFC00) == 0x4400)
        result = specialDataAndBranchExchange(pContext, instr);
    
    pContext->pc = pContext->newPC;
    return result;
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

static ShiftResults Shift_C(uint32_t value, SRType type, uint32_t amount, uint32_t carryIn)
{
    ShiftResults results;

    assert (type != SRType_RRX || amount == 1);
    
    if (amount == 0)
    {
        results.result = value;
        results.carryOut = carryIn;
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
        case SRType_ROR:
            results = ROR_C(value, amount);
            break;
        case SRType_RRX:
            // UNDONE: Actually implement these modes as tests progress.
            assert ( FALSE );
        }
    }
    return results;
}

static ShiftResults LSL_C(uint32_t x, uint32_t shift)
{
    ShiftResults results;
    
    results.carryOut = (shift > 32) ? 0 : x & (1 << (32 - shift));
    results.result = (shift > 31) ? 0 : x << shift;
    return results;
}

static ShiftResults LSR_C(uint32_t x, uint32_t shift)
{
    ShiftResults results;
    
    assert (shift > 0);

    results.carryOut = (shift > 32) ? 0 : (x & (1 << (shift - 1)));
    results.result = (shift > 31) ? 0 : x >> shift;
    return results;
}

static ShiftResults ASR_C(uint32_t x, uint32_t shift)
{
    ShiftResults results;
    
    assert (shift > 0);

    if (shift > 32)
    {
        if (x & 0x80000000)
            results.carryOut = 1;
        else
            results.carryOut = 0;
    }
    else
    {
        results.carryOut = (x & (1 << (shift - 1)));
    }
    if (shift > 31)
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

static ShiftResults ROR_C(uint32_t x, uint32_t shift)
{
    uint32_t     m;
    ShiftResults results;
    
    assert (shift != 0);

    m = shift & 31U;
    results.result = LSR(x, m) | LSL(x, 32-m);
    results.carryOut = results.result & (1 << 31);
    return results;
}

static uint32_t LSR(uint32_t x, uint32_t shift)
{
    ShiftResults results = {0, 0};
    
    assert (shift >= 0);
    
    if (shift == 0)
        results.result = x;
    else
        results = LSR_C(x, shift);
    return results.result;
}

static uint32_t LSL(uint32_t x, uint32_t shift)
{
    ShiftResults results = {0, 0};
    
    assert (shift >= 0);
    
    if (shift == 0)
        results.result = x;
    else
        results = LSL_C(x, shift);
    return results.result;
}

static uint32_t getReg(const PinkySimContext* pContext, uint32_t reg)
{
    assert (reg <= PC);
    
    if (reg == PC)
        return pContext->pc + 4;
    else if (reg == LR)
        return pContext->lr;
    else if (reg == SP)
        // NOTE: Simulator only supports main handler mode.
        return pContext->spMain;
    else
        return pContext->R[reg];
}

static void setReg(PinkySimContext* pContext, uint32_t reg, uint32_t value)
{
    assert (reg < PC);

    if (reg == LR)
        pContext->lr = value;
    else if (reg == SP)
        // NOTE: Simulator only supports main handler mode.
        pContext->spMain = value;
    else
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

static int dataProcessing(PinkySimContext* pContext, uint16_t instr)
{
    switch ((instr & 0x03C0) >> 6)
    {
    case 0:
        return andRegister(pContext, instr);
    case 1:
        return eorRegister(pContext, instr);
    case 2:
        return lslRegister(pContext, instr);
    case 3:
        return lsrRegister(pContext, instr);
    case 4:
        return asrRegister(pContext, instr);
    case 5:
        return adcRegister(pContext, instr);
    case 6:
        return sbcRegister(pContext, instr);
    case 7:
        return rorRegister(pContext, instr);
    case 8:
        return tstRegister(pContext, instr);
    case 9:
        return rsbRegister(pContext, instr);
    case 10:
        return cmpRegisterT1(pContext, instr);
    case 11:
        return cmnRegister(pContext, instr);
    case 12:
        return orrRegister(pContext, instr);
    case 13:
        return mulRegister(pContext, instr);
    case 14:
        return bicRegister(pContext, instr);
    case 15:
        return mvnRegister(pContext, instr);
    default:
        return PINKYSIM_STEP_UNDEFINED;
    }
}

static int andRegister(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t        d = instr & 0x7;
        uint32_t        n = d;
        uint32_t        m = (instr & (0x7 << 3)) >> 3;
        int             setFlags = !InITBlock(pContext);
        DecodedImmShift decodedShift = {SRType_LSL, 0};
        ShiftResults    shiftResults;
        uint32_t        result;
        
        // UNDONE: Only Thumb2 instructions require this shifted value but it is used to make carryOut == carryIn
        shiftResults = Shift_C(getReg(pContext, m), decodedShift.type, decodedShift.n, pContext->xPSR & APSR_C);
        result = getReg(pContext, n) & shiftResults.result;
        setReg(pContext, d, result);
        if (setFlags)
        {
            pContext->xPSR &= ~APSR_NZC;
            if (result & (1 << 31))
                pContext->xPSR |= APSR_N;
            if (result == 0)
                pContext->xPSR |= APSR_Z;
            if (shiftResults.carryOut)
                pContext->xPSR |= APSR_C;
        }
    }

    return PINKYSIM_STEP_OK;
}

static int eorRegister(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t        d = instr & 0x7;
        uint32_t        n = d;
        uint32_t        m = (instr & (0x7 << 3)) >> 3;
        int             setFlags = !InITBlock(pContext);
        DecodedImmShift decodedShift = {SRType_LSL, 0};
        ShiftResults    shiftResults;
        uint32_t        result;
        
        // UNDONE: Only Thumb2 instructions require this shifted value but it is used to make carryOut == carryIn
        shiftResults = Shift_C(getReg(pContext, m), decodedShift.type, decodedShift.n, pContext->xPSR & APSR_C);
        result = getReg(pContext, n) ^ shiftResults.result;
        setReg(pContext, d, result);
        if (setFlags)
        {
            pContext->xPSR &= ~APSR_NZC;
            if (result & (1 << 31))
                pContext->xPSR |= APSR_N;
            if (result == 0)
                pContext->xPSR |= APSR_Z;
            if (shiftResults.carryOut)
                pContext->xPSR |= APSR_C;
        }
    }

    return PINKYSIM_STEP_OK;
}

static int lslRegister(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t        d = instr & 0x7;
        uint32_t        n = d;
        uint32_t        m = (instr & (0x7 << 3)) >> 3;
        int             setFlags = !InITBlock(pContext);
        uint32_t        shiftN;
        ShiftResults    shiftResults;

        shiftN = getReg(pContext, m) & 0xFF;
        shiftResults = Shift_C(getReg(pContext, n), SRType_LSL, shiftN, pContext->xPSR & APSR_C);
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

static int lsrRegister(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t        d = instr & 0x7;
        uint32_t        n = d;
        uint32_t        m = (instr & (0x7 << 3)) >> 3;
        int             setFlags = !InITBlock(pContext);
        uint32_t        shiftN;
        ShiftResults    shiftResults;

        shiftN = getReg(pContext, m) & 0xFF;
        shiftResults = Shift_C(getReg(pContext, n), SRType_LSR, shiftN, pContext->xPSR & APSR_C);
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

static int asrRegister(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t        d = instr & 0x7;
        uint32_t        n = d;
        uint32_t        m = (instr & (0x7 << 3)) >> 3;
        int             setFlags = !InITBlock(pContext);
        uint32_t        shiftN;
        ShiftResults    shiftResults;

        shiftN = getReg(pContext, m) & 0xFF;
        shiftResults = Shift_C(getReg(pContext, n), SRType_ASR, shiftN, pContext->xPSR & APSR_C);
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

static int adcRegister(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t        d = instr & 0x7;
        uint32_t        n = d;
        uint32_t        m = (instr & (0x7 << 3)) >> 3;
        int             setFlags = !InITBlock(pContext);
        DecodedImmShift decodedShift = {SRType_LSL, 0};
        uint32_t        shifted;
        AddResults      addResults;
        
        // UNDONE: Only Thumb2 instructions require this shifted value.
        shifted = Shift(getReg(pContext, m), decodedShift.type, decodedShift.n, pContext->xPSR & APSR_C);
        addResults = AddWithCarry(getReg(pContext, n), shifted, pContext->xPSR & APSR_C);
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

static int sbcRegister(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t        d = instr & 0x7;
        uint32_t        n = d;
        uint32_t        m = (instr & (0x7 << 3)) >> 3;
        int             setFlags = !InITBlock(pContext);
        DecodedImmShift decodedShift = {SRType_LSL, 0};
        uint32_t        shifted;
        AddResults      addResults;
        
        // UNDONE: Only Thumb2 instructions require this shifted value.
        shifted = Shift(getReg(pContext, m), decodedShift.type, decodedShift.n, pContext->xPSR & APSR_C);
        addResults = AddWithCarry(getReg(pContext, n), ~shifted, pContext->xPSR & APSR_C);
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

static int rorRegister(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t        d = instr & 0x7;
        uint32_t        n = d;
        uint32_t        m = (instr & (0x7 << 3)) >> 3;
        int             setFlags = !InITBlock(pContext);
        uint32_t        shiftN;
        ShiftResults    shiftResults;

        shiftN = getReg(pContext, m) & 0xFF;
        shiftResults = Shift_C(getReg(pContext, n), SRType_ROR, shiftN, pContext->xPSR & APSR_C);
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

static int tstRegister(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t        n = instr & 0x7;
        uint32_t        m = (instr & (0x7 << 3)) >> 3;
        DecodedImmShift decodedShift = {SRType_LSL, 0};
        ShiftResults    shiftResults;
        uint32_t        result;
        
        // UNDONE: Only Thumb2 instructions require this shifted value but it is used to make carryOut == carryIn
        shiftResults = Shift_C(getReg(pContext, m), decodedShift.type, decodedShift.n, pContext->xPSR & APSR_C);
        result = getReg(pContext, n) & shiftResults.result;
        pContext->xPSR &= ~APSR_NZC;
        if (result & (1 << 31))
            pContext->xPSR |= APSR_N;
        if (result == 0)
            pContext->xPSR |= APSR_Z;
        if (shiftResults.carryOut)
            pContext->xPSR |= APSR_C;
    }

    return PINKYSIM_STEP_OK;
}

static int rsbRegister(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t        d = instr & 0x7;
        uint32_t        n = (instr & (0x7 << 3)) >> 3;
        int             setFlags = !InITBlock(pContext);
        uint32_t        imm32 = 0;
        AddResults      addResults;
        
        addResults = AddWithCarry(~getReg(pContext, n), imm32, 1);
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

static int cmpRegisterT1(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t        n = instr & 0x7;
        uint32_t        m = (instr & (0x7 << 3)) >> 3;
        DecodedImmShift decodedShift = {SRType_LSL, 0};
        uint32_t        shifted;
        AddResults      addResults;
        
        // UNDONE: Only Thumb2 instructions require this shifted value.
        shifted = Shift(getReg(pContext, m), decodedShift.type, decodedShift.n, pContext->xPSR & APSR_C);
        addResults = AddWithCarry(getReg(pContext, n), ~shifted, 1);
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

static int cmnRegister(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t        n = instr & 0x7;
        uint32_t        m = (instr & (0x7 << 3)) >> 3;
        DecodedImmShift decodedShift = {SRType_LSL, 0};
        uint32_t        shifted;
        AddResults      addResults;
        
        // UNDONE: Only Thumb2 instructions require this shifted value.
        shifted = Shift(getReg(pContext, m), decodedShift.type, decodedShift.n, pContext->xPSR & APSR_C);
        addResults = AddWithCarry(getReg(pContext, n), shifted, 0);
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

static int orrRegister(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t        d = instr & 0x7;
        uint32_t        n = d;
        uint32_t        m = (instr & (0x7 << 3)) >> 3;
        int             setFlags = !InITBlock(pContext);
        DecodedImmShift decodedShift = {SRType_LSL, 0};
        ShiftResults    shiftResults;
        uint32_t        result;
        
        // UNDONE: Only Thumb2 instructions require this shifted value but it is used to make carryOut == carryIn
        shiftResults = Shift_C(getReg(pContext, m), decodedShift.type, decodedShift.n, pContext->xPSR & APSR_C);
        result = getReg(pContext, n) | shiftResults.result;
        setReg(pContext, d, result);
        if (setFlags)
        {
            pContext->xPSR &= ~APSR_NZC;
            if (result & (1 << 31))
                pContext->xPSR |= APSR_N;
            if (result == 0)
                pContext->xPSR |= APSR_Z;
            if (shiftResults.carryOut)
                pContext->xPSR |= APSR_C;
        }
    }

    return PINKYSIM_STEP_OK;
}

static int mulRegister(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t        d = instr & 0x7;
        uint32_t        m = d;
        uint32_t        n = (instr & (0x7 << 3)) >> 3;
        int             setFlags = !InITBlock(pContext);
        uint32_t        operand1 = getReg(pContext, n);
        uint32_t        operand2 = getReg(pContext, m);
        uint32_t        result = operand1 * operand2;
        
        setReg(pContext, d, result);
        if (setFlags)
        {
            pContext->xPSR &= ~APSR_NZ;
            if (result & (1 << 31))
                pContext->xPSR |= APSR_N;
            if (result == 0)
                pContext->xPSR |= APSR_Z;
        }
    }

    return PINKYSIM_STEP_OK;
}

static int bicRegister(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t        d = instr & 0x7;
        uint32_t        n = d;
        uint32_t        m = (instr & (0x7 << 3)) >> 3;
        int             setFlags = !InITBlock(pContext);
        DecodedImmShift decodedShift = {SRType_LSL, 0};
        ShiftResults    shiftResults;
        uint32_t        result;
        
        // UNDONE: Only Thumb2 instructions require this shifted value but it is used to make carryOut == carryIn
        shiftResults = Shift_C(getReg(pContext, m), decodedShift.type, decodedShift.n, pContext->xPSR & APSR_C);
        result = getReg(pContext, n) & ~shiftResults.result;
        setReg(pContext, d, result);
        if (setFlags)
        {
            pContext->xPSR &= ~APSR_NZC;
            if (result & (1 << 31))
                pContext->xPSR |= APSR_N;
            if (result == 0)
                pContext->xPSR |= APSR_Z;
            if (shiftResults.carryOut)
                pContext->xPSR |= APSR_C;
        }
    }

    return PINKYSIM_STEP_OK;
}

static int mvnRegister(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t        d = instr & 0x7;
        uint32_t        m = (instr & (0x7 << 3)) >> 3;
        int             setFlags = !InITBlock(pContext);
        DecodedImmShift decodedShift = {SRType_LSL, 0};
        ShiftResults    shiftResults;
        uint32_t        result;
        
        // UNDONE: Only Thumb2 instructions require this shifted value but it is used to make carryOut == carryIn
        shiftResults = Shift_C(getReg(pContext, m), decodedShift.type, decodedShift.n, pContext->xPSR & APSR_C);
        result = ~shiftResults.result;
        setReg(pContext, d, result);
        if (setFlags)
        {
            pContext->xPSR &= ~APSR_NZC;
            if (result & (1 << 31))
                pContext->xPSR |= APSR_N;
            if (result == 0)
                pContext->xPSR |= APSR_Z;
            if (shiftResults.carryOut)
                pContext->xPSR |= APSR_C;
        }
    }

    return PINKYSIM_STEP_OK;
}

static int specialDataAndBranchExchange(PinkySimContext* pContext, uint16_t instr)
{
    if ((instr & 0x0300) == 0x0000)
        return addRegisterT2(pContext, instr);
    else if ((instr & 0x03C0) == 0x0100)
        return PINKYSIM_STEP_UNPREDICTABLE;
    else if (((instr & 0x03C0) == 0x0140) ||
             ((instr & 0x0380) == 0x0180))
        return cmpRegisterT2(pContext, instr);
    else if ((instr & 0x0300) == 0x0200)
        return movRegister(pContext, instr);
    else if ((instr & 0x0380) == 0x0300)
        return bx(pContext, instr);
    else if ((instr & 0x0380) == 0x0380)
        return blx(pContext, instr);
    else
        return PINKYSIM_STEP_UNDEFINED;
}

static int addRegisterT2(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t        d = ((instr & (1 << 7)) >> 4) | (instr & 0x7);
        uint32_t        n = d;
        uint32_t        m = (instr & (0xF << 3)) >> 3;
        // UNDONE: Not required for this encoding.
        //int             setFlags = FALSE;
        DecodedImmShift decodedShift = {SRType_LSL, 0};
        uint32_t        shifted;
        AddResults      addResults;

        if (d == 15 && m == 15)
            return PINKYSIM_STEP_UNPREDICTABLE;
        // UNDONE: InITBlock() always returns FALSE for ARMv6-m so this assert will never fire.
        //if (d == 15 && InITBlock(pContext) && !LastInITBlock(pContext))
        //    return PINKYSIM_STEP_UNPREDICTABLE;
        
        // UNDONE: Only Thumb2 instructions require this shifted value.
        shifted = Shift(getReg(pContext, m), decodedShift.type, decodedShift.n, pContext->xPSR & APSR_C);
        addResults = AddWithCarry(getReg(pContext, n), shifted, 0);
        if (d == 15)
        {
            ALUWritePC(pContext, addResults.result);
        }
        else
        {
            setReg(pContext, d, addResults.result);
            // UNDONE: setFlags is forced to FALSE for this encoding.
#ifdef UNDONE
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
#endif // UNDONE
        }
    }

    return PINKYSIM_STEP_OK;
}

static void ALUWritePC(PinkySimContext* pContext, uint32_t address)
{
    BranchWritePC(pContext, address);
}

static void BranchWritePC(PinkySimContext* pContext, uint32_t address)
{
    BranchTo(pContext, address & 0xFFFFFFFE);
}

static void BranchTo(PinkySimContext* pContext, uint32_t address)
{
    pContext->newPC = address;
}

static int cmpRegisterT2(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t        n = ((instr & (1 << 7)) >> 4) | (instr & 0x7);
        uint32_t        m = (instr & (0xF << 3)) >> 3;
        DecodedImmShift decodedShift = {SRType_LSL, 0};
        uint32_t        shifted;
        AddResults      addResults;
        
        // UNDONE: This scenario is caught up above in specialDataAndBranchExchange()
        //if (n < 8 && m < 8)
        //    return PINKYSIM_STEP_UNPREDICTABLE;
        if (n == 15 || m == 15)
            return PINKYSIM_STEP_UNPREDICTABLE;
        
        // UNDONE: Only Thumb2 instructions require this shifted value.
        shifted = Shift(getReg(pContext, m), decodedShift.type, decodedShift.n, pContext->xPSR & APSR_C);
        addResults = AddWithCarry(getReg(pContext, n), ~shifted, 1);
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

static int movRegister(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t        d = ((instr & (1 << 7)) >> 4) | (instr & 0x7);
        uint32_t        m = (instr & (0xF << 3)) >> 3;
        // UNDONE: Not required for this encoding.
        //int             setFlags = FALSE;
        uint32_t        result;

        result = getReg(pContext, m);
        if (d == 15)
        {
            ALUWritePC(pContext, result);
        }
        else
        {
            setReg(pContext, d, result);
            // UNDONE: setFlags is forced to FALSE for this encoding.
#ifdef UNDONE
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
#endif // UNDONE
        }
    }

    return PINKYSIM_STEP_OK;
}

static int bx(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t        m = (instr & (0xF << 3)) >> 3;
        
        // UNDONE: InITBlock() always returns FALSE for ARMv6-m so this assert will never fire.
        //if (InITBlock(pContext) && !LastInITBlock(pContext))
        //    return PINKYSIM_STEP_UNPREDICTABLE;
        if ((instr & 0x7) != 0x0)
            return PINKYSIM_STEP_UNPREDICTABLE;
        if (m == 15)
            return PINKYSIM_STEP_UNPREDICTABLE;
        
        BXWritePC(pContext, getReg(pContext, m));
    }

    return PINKYSIM_STEP_OK;
}

static void BXWritePC(PinkySimContext* pContext, uint32_t address)
{
    // UNDONE: Don't support exception handlers in this simulator so no need to check for return from exception handler.
    //if (pContext->currentMode == modeHandler && (address & 0xF000) == 0xF000)
    //    ExceptionReturn(pContext, address & ((1 << 27) - 1));
    // else
    pContext->xPSR &= ~EPSR_T;
    if (address & 1)
        pContext->xPSR |= EPSR_T;
    BranchTo(pContext, address & 0xFFFFFFFE);
}

static int blx(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t m = (instr & (0xF << 3)) >> 3;
        uint32_t target;
        uint32_t nextInstrAddr;
        
        // UNDONE: InITBlock() always returns FALSE for ARMv6-m so this assert will never fire.
        //if (InITBlock(pContext) && !LastInITBlock(pContext))
        //    return PINKYSIM_STEP_UNPREDICTABLE;
        if ((instr & 0x7) != 0x0)
            return PINKYSIM_STEP_UNPREDICTABLE;
        if (m == 15)
            return PINKYSIM_STEP_UNPREDICTABLE;
        
        target = getReg(pContext, m);
        nextInstrAddr = getReg(pContext, PC) - 2;
        setReg(pContext, LR, nextInstrAddr | 1);
        BLXWritePC(pContext, target);
    }

    return PINKYSIM_STEP_OK;
}

static void BLXWritePC(PinkySimContext* pContext, uint32_t address)
{
    pContext->xPSR &= ~EPSR_T;
    if (address & 1)
        pContext->xPSR |= EPSR_T;
    BranchTo(pContext, address & 0xFFFFFFFE);
}

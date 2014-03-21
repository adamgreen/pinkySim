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
static int ldrLiteral(PinkySimContext* pContext, uint16_t instr);
static uint32_t Align(uint32_t value, uint32_t alignment);
static uint32_t unalignedMemRead(PinkySimContext* pContext, uint32_t address, uint32_t size);
static uint32_t alignedMemRead(PinkySimContext* pContext, uint32_t address, uint32_t size);
static int isAligned(uint32_t address, uint32_t size);
static int loadStoreSingleDataItem(PinkySimContext* pContext, uint16_t instr);
static int strRegister(PinkySimContext* pContext, uint16_t instr);
static void unalignedMemWrite(PinkySimContext* pContext, uint32_t address, uint32_t size, uint32_t value);
static void alignedMemWrite(PinkySimContext* pContext, uint32_t address, uint32_t size, uint32_t value);
static int strhRegister(PinkySimContext* pContext, uint16_t instr);
static int strbRegister(PinkySimContext* pContext, uint16_t instr);
static int ldrsbRegister(PinkySimContext* pContext, uint16_t instr);
static uint32_t signExtend8(uint32_t valueToExtend);
static int ldrRegister(PinkySimContext* pContext, uint16_t instr);
static int ldrhRegister(PinkySimContext* pContext, uint16_t instr);
static uint32_t zeroExtend16(uint32_t value);
static int ldrbRegister(PinkySimContext* pContext, uint16_t instr);
static uint32_t zeroExtend8(uint32_t value);
static int ldrshRegister(PinkySimContext* pContext, uint16_t instr);
static uint32_t signExtend16(uint32_t valueToExtend);
static int strImmediateT1(PinkySimContext* pContext, uint16_t instr);
static int ldrImmediateT1(PinkySimContext* pContext, uint16_t instr);
static int strbImmediate(PinkySimContext* pContext, uint16_t instr);
static int ldrbImmediate(PinkySimContext* pContext, uint16_t instr);
static int strhImmediate(PinkySimContext* pContext, uint16_t instr);
static int ldrhImmediate(PinkySimContext* pContext, uint16_t instr);
static int strImmediateT2(PinkySimContext* pContext, uint16_t instr);
static int ldrImmediateT2(PinkySimContext* pContext, uint16_t instr);
static int adr(PinkySimContext* pContext, uint16_t instr);
static int addSPT1(PinkySimContext* pContext, uint16_t instr);
static int misc16BitInstructions(PinkySimContext* pContext, uint16_t instr);
static int addSPT2(PinkySimContext* pContext, uint16_t instr);
static int subSP(PinkySimContext* pContext, uint16_t instr);
static int sxth(PinkySimContext* pContext, uint16_t instr);
static uint32_t ROR(uint32_t x, uint32_t shift);
static int sxtb(PinkySimContext* pContext, uint16_t instr);
static int uxth(PinkySimContext* pContext, uint16_t instr);
static int uxtb(PinkySimContext* pContext, uint16_t instr);
static int push(PinkySimContext* pContext, uint16_t instr);
static uint32_t bitCount(uint32_t value);
static int cps(PinkySimContext* pContext, uint16_t instr);
static int currentModeIsPrivileged(PinkySimContext* pContext);
static int rev(PinkySimContext* pContext, uint16_t instr);


int pinkySimStep(PinkySimContext* pContext)
{
    int      result = PINKYSIM_STEP_UNDEFINED;

    if (!(pContext->xPSR & EPSR_T))
        return PINKYSIM_STEP_HARDFAULT;

    __try
    {
        uint16_t instr =  IMemory_Read16(pContext->pMemory, pContext->pc);
        // UNDONE: This is specific to 16-bit instructions.
        pContext->newPC = pContext->pc + 2;

        if ((instr & 0xC000) == 0x0000)
            result = shiftAddSubtractMoveCompare(pContext, instr);
        else if ((instr & 0xFC00) == 0x4000)
            result = dataProcessing(pContext, instr);
        else if ((instr & 0xFC00) == 0x4400)
            result = specialDataAndBranchExchange(pContext, instr);
        else if ((instr & 0xF800) == 0x4800)
            result = ldrLiteral(pContext, instr);
        else if (((instr & 0xF000) == 0x5000) || ((instr & 0xE000) == 0x6000) || ((instr & 0xE000) == 0x8000))
            result = loadStoreSingleDataItem(pContext, instr);
        else if ((instr & 0xF800) == 0xA000)
            result = adr(pContext, instr);
        else if ((instr & 0xF800) == 0xA800)
            result = addSPT1(pContext, instr);
        else if ((instr & 0xF000) == 0xB000)
            result = misc16BitInstructions(pContext, instr);
    
        pContext->pc = pContext->newPC;
    }
    __catch
    {
        return PINKYSIM_STEP_HARDFAULT;
    }
    
    return result;
}

static int shiftAddSubtractMoveCompare(PinkySimContext* pContext, uint16_t instr)
{
    int result = PINKYSIM_STEP_UNDEFINED;

    if ((instr & 0x3800) == 0x0000)
        result = lslImmediate(pContext, instr);
    else if ((instr & 0x3800) == 0x0800)
        result = lsrImmediate(pContext, instr);
    else if ((instr & 0x3800) == 0x1000)
        result = asrImmediate(pContext, instr);
    else if ((instr & 0x3E00) == 0x1800)
        result = addRegisterT1(pContext, instr);
    else if ((instr & 0x3E00) == 0x1A00)
        result = subRegister(pContext, instr);
    else if ((instr & 0x3E00) == 0x1C00)
        result = addImmediateT1(pContext, instr);
    else if ((instr & 0x3E00) == 0x1E00)
        result = subImmediateT1(pContext, instr);
    else if ((instr & 0x3800) == 0x2000)
        result = movImmediate(pContext, instr);
    else if ((instr & 0x3800) == 0x2800)
        result = cmpImmediate(pContext, instr);
    else if ((instr & 0x3800) == 0x3000)
        result = addImmediateT2(pContext, instr);
    else if ((instr & 0x3800) == 0x3800)
        result = subImmediateT2(pContext, instr);

    return result;
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
    int result = PINKYSIM_STEP_UNDEFINED;
    
    switch ((instr & 0x03C0) >> 6)
    {
    case 0:
        result = andRegister(pContext, instr);
        break;
    case 1:
        result = eorRegister(pContext, instr);
        break;
    case 2:
        result = lslRegister(pContext, instr);
        break;
    case 3:
        result = lsrRegister(pContext, instr);
        break;
    case 4:
        result = asrRegister(pContext, instr);
        break;
    case 5:
        result = adcRegister(pContext, instr);
        break;
    case 6:
        result = sbcRegister(pContext, instr);
        break;
    case 7:
        result = rorRegister(pContext, instr);
        break;
    case 8:
        result = tstRegister(pContext, instr);
        break;
    case 9:
        result = rsbRegister(pContext, instr);
        break;
    case 10:
        result = cmpRegisterT1(pContext, instr);
        break;
    case 11:
        result = cmnRegister(pContext, instr);
        break;
    case 12:
        result = orrRegister(pContext, instr);
        break;
    case 13:
        result = mulRegister(pContext, instr);
        break;
    case 14:
        result = bicRegister(pContext, instr);
        break;
    case 15:
        result = mvnRegister(pContext, instr);
        break;
    }
    
    return result;
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
    int result = PINKYSIM_STEP_UNDEFINED;
    
    if ((instr & 0x0300) == 0x0000)
        result = addRegisterT2(pContext, instr);
    else if ((instr & 0x03C0) == 0x0100)
        result = PINKYSIM_STEP_UNPREDICTABLE;
    else if (((instr & 0x03C0) == 0x0140) || ((instr & 0x0380) == 0x0180))
        result = cmpRegisterT2(pContext, instr);
    else if ((instr & 0x0300) == 0x0200)
        result = movRegister(pContext, instr);
    else if ((instr & 0x0380) == 0x0300)
        result = bx(pContext, instr);
    else if ((instr & 0x0380) == 0x0380)
        result = blx(pContext, instr);
        
    return result;
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

static int ldrLiteral(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t t = (instr & (0x7 << 8)) >> 8;
        uint32_t imm32 = (instr & 0xFF) << 2;
        // UNDONE: Add is forced to be TRUE in the ARMv6-m encoding.
        //int      add = TRUE;
        uint32_t base;
        uint32_t address;

        base = Align(getReg(pContext, PC), 4);
        // UNDONE: Add is forced to be TRUE in the ARMv6-m encoding.
        //if (add)
            address = base + imm32;
        //else
        //    address = base - imm32;
        setReg(pContext, t, unalignedMemRead(pContext, address, 4));
    }

    return PINKYSIM_STEP_OK;
}

static uint32_t Align(uint32_t value, uint32_t alignment)
{
    assert (alignment == 2 || alignment == 4);
    return value & ~(alignment - 1);
}

static uint32_t unalignedMemRead(PinkySimContext* pContext, uint32_t address, uint32_t size)
{
    // UNDONE: All memory accesses must be aligned on ARMv6-M.
    return alignedMemRead(pContext, address, size);
}

static uint32_t alignedMemRead(PinkySimContext* pContext, uint32_t address, uint32_t size)
{
    uint32_t result = 0xFFFFFFFF;
    
    assert (size == 4 || size == 2 || size == 1);

    if (!isAligned(address, size))
        __throw(alignmentException);
        
    switch (size)
    {
    case 1:
        result = IMemory_Read8(pContext->pMemory, address);
        break;
    case 2:
        result = IMemory_Read16(pContext->pMemory, address);
        break;
    case 4:
        result = IMemory_Read32(pContext->pMemory, address);
        break;
    }
    return result;
}

static int isAligned(uint32_t address, uint32_t size)
{
    assert (size == 4 || size == 2 || size == 1);

    return address == (address & ~(size - 1));
}

static int loadStoreSingleDataItem(PinkySimContext* pContext, uint16_t instr)
{
    int result = PINKYSIM_STEP_UNDEFINED;
    
    if ((instr & 0xFE00) == 0x5000)
        result = strRegister(pContext, instr);
    else if ((instr & 0xFE00) == 0x5200)
        result = strhRegister(pContext, instr);
    else if ((instr & 0xFE00) == 0x5400)
        result = strbRegister(pContext, instr);
    else if ((instr & 0xFE00) == 0x5600)
        result = ldrsbRegister(pContext, instr);
    else if ((instr & 0xFE00) == 0x5800)
        result = ldrRegister(pContext, instr);
    else if ((instr & 0xFE00) == 0x5A00)
        result = ldrhRegister(pContext, instr);
    else if ((instr & 0xFE00) == 0x5C00)
        result = ldrbRegister(pContext, instr);
    else if ((instr & 0xFE00) == 0x5E00)
        result = ldrshRegister(pContext, instr);
    else if ((instr & 0xF800) == 0x6000)
        result = strImmediateT1(pContext, instr);
    else if ((instr & 0xF800) == 0x6800)
        result = ldrImmediateT1(pContext, instr);
    else if ((instr & 0xF800) == 0x7000)
        result = strbImmediate(pContext, instr);
    else if ((instr & 0xF800) == 0x7800)
        result = ldrbImmediate(pContext, instr);
    else if ((instr & 0xF800) == 0x8000)
        result = strhImmediate(pContext, instr);
    else if ((instr & 0xF800) == 0x8800)
        result = ldrhImmediate(pContext, instr);
    else if ((instr & 0xF800) == 0x9000)
        result = strImmediateT2(pContext, instr);
    else if ((instr & 0xF800) == 0x9800)
        result = ldrImmediateT2(pContext, instr);
        
    return result;
}

static int strRegister(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t        t = instr & 0x7;
        uint32_t        n = (instr & (0x7 << 3)) >> 3;
        uint32_t        m = (instr & (0x7 << 6)) >> 6;
        DecodedImmShift decodedShift = {SRType_LSL, 0};
        uint32_t        offset;
        uint32_t        address;
        
        // UNDONE: Only Thumb2 instructions require this shifted value.
        offset = Shift(getReg(pContext, m), decodedShift.type, decodedShift.n, pContext->xPSR & APSR_C);
        address = getReg(pContext, n) + offset;
        unalignedMemWrite(pContext, address, 4, getReg(pContext, t));
    }

    return PINKYSIM_STEP_OK;
}

static void unalignedMemWrite(PinkySimContext* pContext, uint32_t address, uint32_t size, uint32_t value)
{
    // UNDONE: All memory accesses must be aligned on ARMv6-M.
    alignedMemWrite(pContext, address, size, value);
}

static void alignedMemWrite(PinkySimContext* pContext, uint32_t address, uint32_t size, uint32_t value)
{
    assert (size == 4 || size == 2 || size == 1);

    if (!isAligned(address, size))
        __throw(alignmentException);
    switch (size)
    {
    case 4:
        IMemory_Write32(pContext->pMemory, address, value);
        break;
    case 2:
        IMemory_Write16(pContext->pMemory, address, value);
        break;
    case 1:
        IMemory_Write8(pContext->pMemory, address, value);
        break;
    }
}

static int strhRegister(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t        t = instr & 0x7;
        uint32_t        n = (instr & (0x7 << 3)) >> 3;
        uint32_t        m = (instr & (0x7 << 6)) >> 6;
        DecodedImmShift decodedShift = {SRType_LSL, 0};
        uint32_t        offset;
        uint32_t        address;
        
        // UNDONE: Only Thumb2 instructions require this shifted value.
        offset = Shift(getReg(pContext, m), decodedShift.type, decodedShift.n, pContext->xPSR & APSR_C);
        address = getReg(pContext, n) + offset;
        unalignedMemWrite(pContext, address, 2, getReg(pContext, t));
    }

    return PINKYSIM_STEP_OK;
}

static int strbRegister(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t        t = instr & 0x7;
        uint32_t        n = (instr & (0x7 << 3)) >> 3;
        uint32_t        m = (instr & (0x7 << 6)) >> 6;
        DecodedImmShift decodedShift = {SRType_LSL, 0};
        uint32_t        offset;
        uint32_t        address;
        
        // UNDONE: Only Thumb2 instructions require this shifted value.
        offset = Shift(getReg(pContext, m), decodedShift.type, decodedShift.n, pContext->xPSR & APSR_C);
        address = getReg(pContext, n) + offset;
        unalignedMemWrite(pContext, address, 1, getReg(pContext, t));
    }

    return PINKYSIM_STEP_OK;
}

static int ldrsbRegister(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t        t = instr & 0x7;
        uint32_t        n = (instr & (0x7 << 3)) >> 3;
        uint32_t        m = (instr & (0x7 << 6)) >> 6;
        // UNDONE: Not required for ARMv6-M encodings.
        //int             index = TRUE;
        //int             add = TRUE;
        //int             wback = TRUE;
        DecodedImmShift decodedShift = {SRType_LSL, 0};
        uint32_t        offset;
        uint32_t        offsetAddress;
        uint32_t        address;
        
        // UNDONE: Only Thumb2 instructions require this shifted value.
        offset = Shift(getReg(pContext, m), decodedShift.type, decodedShift.n, pContext->xPSR & APSR_C);
        // UNDONE: Not required for ARMv6-m encodings.
        //if (add)
            offsetAddress = getReg(pContext, n) + offset;
        //else
        //    offsetAddress = getReg(pContext, n) - offset;
        //if (index)
            address = offsetAddress;
        //else
        //    address = getReg(pContext, n);
        setReg(pContext, t, signExtend8(unalignedMemRead(pContext, address, 1)));
    }

    return PINKYSIM_STEP_OK;
}

static uint32_t signExtend8(uint32_t valueToExtend)
{
    return (uint32_t)(((int32_t)valueToExtend << 24) >> 24);
}

static int ldrRegister(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t        t = instr & 0x7;
        uint32_t        n = (instr & (0x7 << 3)) >> 3;
        uint32_t        m = (instr & (0x7 << 6)) >> 6;
        // UNDONE: Not required for ARMv6-M encodings.
        //int             index = TRUE;
        //int             add = TRUE;
        //int             wback = FALSE;
        DecodedImmShift decodedShift = {SRType_LSL, 0};
        uint32_t        offset;
        uint32_t        offsetAddress;
        uint32_t        address;
        
        // UNDONE: Only Thumb2 instructions require this shifted value.
        offset = Shift(getReg(pContext, m), decodedShift.type, decodedShift.n, pContext->xPSR & APSR_C);
        // UNDONE: Not required for ARMv6-m encodings.
        //if (add)
            offsetAddress = getReg(pContext, n) + offset;
        //else
        //    offsetAddress = getReg(pContext, n) - offset;
        //if (index)
            address = offsetAddress;
        //else
        //    address = getReg(pContext, n);
        setReg(pContext, t, unalignedMemRead(pContext, address, 4));
        //if (wback)
        //    setReg(pContext, n, offsetAddress);
    }

    return PINKYSIM_STEP_OK;
}

static int ldrhRegister(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t        t = instr & 0x7;
        uint32_t        n = (instr & (0x7 << 3)) >> 3;
        uint32_t        m = (instr & (0x7 << 6)) >> 6;
        // UNDONE: Not required for ARMv6-M encodings.
        //int             index = TRUE;
        //int             add = TRUE;
        //int             wback = FALSE;
        DecodedImmShift decodedShift = {SRType_LSL, 0};
        uint32_t        offset;
        uint32_t        offsetAddress;
        uint32_t        address;
        uint32_t        data;
        
        // UNDONE: Only Thumb2 instructions require this shifted value.
        offset = Shift(getReg(pContext, m), decodedShift.type, decodedShift.n, pContext->xPSR & APSR_C);
        // UNDONE: Not required for ARMv6-m encodings.
        //if (add)
            offsetAddress = getReg(pContext, n) + offset;
        //else
        //    offsetAddress = getReg(pContext, n) - offset;
        //if (index)
            address = offsetAddress;
        //else
        //    address = getReg(pContext, n);
        data = unalignedMemRead(pContext, address, 2);
        //if (wback)
        //    setReg(pContext, n, offsetAddress);
        setReg(pContext, t, zeroExtend16(data));
    }

    return PINKYSIM_STEP_OK;
}

static uint32_t zeroExtend16(uint32_t value)
{
    return value & 0xFFFF;
}

static int ldrbRegister(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t        t = instr & 0x7;
        uint32_t        n = (instr & (0x7 << 3)) >> 3;
        uint32_t        m = (instr & (0x7 << 6)) >> 6;
        // UNDONE: Not required for ARMv6-M encodings.
        //int             index = TRUE;
        //int             add = TRUE;
        //int             wback = FALSE;
        DecodedImmShift decodedShift = {SRType_LSL, 0};
        uint32_t        offset;
        uint32_t        offsetAddress;
        uint32_t        address;
        
        // UNDONE: Only Thumb2 instructions require this shifted value.
        offset = Shift(getReg(pContext, m), decodedShift.type, decodedShift.n, pContext->xPSR & APSR_C);
        // UNDONE: Not required for ARMv6-m encodings.
        //if (add)
            offsetAddress = getReg(pContext, n) + offset;
        //else
        //    offsetAddress = getReg(pContext, n) - offset;
        //if (index)
            address = offsetAddress;
        //else
        //    address = getReg(pContext, n);
        setReg(pContext, t, zeroExtend8(unalignedMemRead(pContext, address, 1)));
    }

    return PINKYSIM_STEP_OK;
}

static uint32_t zeroExtend8(uint32_t value)
{
    return value & 0xFF;
}

static int ldrshRegister(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t        t = instr & 0x7;
        uint32_t        n = (instr & (0x7 << 3)) >> 3;
        uint32_t        m = (instr & (0x7 << 6)) >> 6;
        // UNDONE: Not required for ARMv6-M encodings.
        //int             index = TRUE;
        //int             add = TRUE;
        //int             wback = FALSE;
        DecodedImmShift decodedShift = {SRType_LSL, 0};
        uint32_t        offset;
        uint32_t        offsetAddress;
        uint32_t        address;
        uint32_t        data;
        
        // UNDONE: Only Thumb2 instructions require this shifted value.
        offset = Shift(getReg(pContext, m), decodedShift.type, decodedShift.n, pContext->xPSR & APSR_C);
        // UNDONE: Not required for ARMv6-m encodings.
        //if (add)
            offsetAddress = getReg(pContext, n) + offset;
        //else
        //    offsetAddress = getReg(pContext, n) - offset;
        //if (index)
            address = offsetAddress;
        //else
        //    address = getReg(pContext, n);
        data = unalignedMemRead(pContext, address, 2);
        //if (wback)
        //    setReg(pContext, n, offsetAddress);
        setReg(pContext, t, signExtend16(data));
    }

    return PINKYSIM_STEP_OK;
}

static uint32_t signExtend16(uint32_t valueToExtend)
{
    return (uint32_t)(((int32_t)valueToExtend << 16) >> 16);
}

static int strImmediateT1(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t        t = instr & 0x7;
        uint32_t        n = (instr & (0x7 << 3)) >> 3;
        uint32_t        imm32 = (instr & (0x1F << 6)) >> 4;
        // UNDONE: Not required for ARMv6-M encodings.
        //int             index = TRUE;
        //int             add = TRUE;
        //int             wback = FALSE;        
        uint32_t        offsetAddress;
        uint32_t        address;

        // UNDONE: Not required for ARMv6-m encodings.
        //if (add)
            offsetAddress = getReg(pContext, n) + imm32;
        //else
        //    offsetAddress = getReg(pContext, n) - imm32;
        //if (index)
            address = offsetAddress;
        //else
        //    address = getReg(pContext, n);
        unalignedMemWrite(pContext, address, 4, getReg(pContext, t));
        //if (wback)
        //    setReg(pContext, n, offsetAddress);
    }

    return PINKYSIM_STEP_OK;
}

static int ldrImmediateT1(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t        t = instr & 0x7;
        uint32_t        n = (instr & (0x7 << 3)) >> 3;
        uint32_t        imm32 = (instr & (0x1F << 6)) >> 4;
        // UNDONE: Not required for ARMv6-M encodings.
        //int             index = TRUE;
        //int             add = TRUE;
        //int             wback = FALSE;        
        uint32_t        offsetAddress;
        uint32_t        address;

        // UNDONE: Not required for ARMv6-m encodings.
        //if (add)
            offsetAddress = getReg(pContext, n) + imm32;
        //else
        //    offsetAddress = getReg(pContext, n) - imm32;
        //if (index)
            address = offsetAddress;
        //else
        //    address = getReg(pContext, n);
        setReg(pContext, t, unalignedMemRead(pContext, address, 4));
        //if (wback)
        //    setReg(pContext, n, offsetAddress);
    }

    return PINKYSIM_STEP_OK;
}

static int strbImmediate(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t        t = instr & 0x7;
        uint32_t        n = (instr & (0x7 << 3)) >> 3;
        uint32_t        imm32 = (instr & (0x1F << 6)) >> 6;
        // UNDONE: Not required for ARMv6-M encodings.
        //int             index = TRUE;
        //int             add = TRUE;
        //int             wback = FALSE;        
        uint32_t        offsetAddress;
        uint32_t        address;

        // UNDONE: Not required for ARMv6-m encodings.
        //if (add)
            offsetAddress = getReg(pContext, n) + imm32;
        //else
        //    offsetAddress = getReg(pContext, n) - imm32;
        //if (index)
            address = offsetAddress;
        //else
        //    address = getReg(pContext, n);
        unalignedMemWrite(pContext, address, 1, getReg(pContext, t));
        //if (wback)
        //    setReg(pContext, n, offsetAddress);
    }

    return PINKYSIM_STEP_OK;
}

static int ldrbImmediate(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t        t = instr & 0x7;
        uint32_t        n = (instr & (0x7 << 3)) >> 3;
        uint32_t        imm32 = (instr & (0x1F << 6)) >> 6;
        // UNDONE: Not required for ARMv6-M encodings.
        //int             index = TRUE;
        //int             add = TRUE;
        //int             wback = FALSE;        
        uint32_t        offsetAddress;
        uint32_t        address;

        // UNDONE: Not required for ARMv6-m encodings.
        //if (add)
            offsetAddress = getReg(pContext, n) + imm32;
        //else
        //    offsetAddress = getReg(pContext, n) - imm32;
        //if (index)
            address = offsetAddress;
        //else
        //    address = getReg(pContext, n);
        setReg(pContext, t, unalignedMemRead(pContext, address, 1));
        //if (wback)
        //    setReg(pContext, n, offsetAddress);
    }

    return PINKYSIM_STEP_OK;
}

static int strhImmediate(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t        t = instr & 0x7;
        uint32_t        n = (instr & (0x7 << 3)) >> 3;
        uint32_t        imm32 = (instr & (0x1F << 6)) >> 5;
        // UNDONE: Not required for ARMv6-M encodings.
        //int             index = TRUE;
        //int             add = TRUE;
        //int             wback = FALSE;        
        uint32_t        offsetAddress;
        uint32_t        address;

        // UNDONE: Not required for ARMv6-m encodings.
        //if (add)
            offsetAddress = getReg(pContext, n) + imm32;
        //else
        //    offsetAddress = getReg(pContext, n) - imm32;
        //if (index)
            address = offsetAddress;
        //else
        //    address = getReg(pContext, n);
        unalignedMemWrite(pContext, address, 2, getReg(pContext, t));
        //if (wback)
        //    setReg(pContext, n, offsetAddress);
    }

    return PINKYSIM_STEP_OK;
}

static int ldrhImmediate(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t        t = instr & 0x7;
        uint32_t        n = (instr & (0x7 << 3)) >> 3;
        uint32_t        imm32 = (instr & (0x1F << 6)) >> 5;
        // UNDONE: Not required for ARMv6-M encodings.
        //int             index = TRUE;
        //int             add = TRUE;
        //int             wback = FALSE;        
        uint32_t        offsetAddress;
        uint32_t        address;

        // UNDONE: Not required for ARMv6-m encodings.
        //if (add)
            offsetAddress = getReg(pContext, n) + imm32;
        //else
        //    offsetAddress = getReg(pContext, n) - imm32;
        //if (index)
            address = offsetAddress;
        //else
        //    address = getReg(pContext, n);
        //if (wback)
        //    setReg(pContext, n, offsetAddress);
        setReg(pContext, t, unalignedMemRead(pContext, address, 2));
    }

    return PINKYSIM_STEP_OK;
}

static int strImmediateT2(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t        t = (instr & (0x7 << 8)) >> 8;
        uint32_t        n = SP;
        uint32_t        imm32 = (instr & 0xFF) << 2;
        // UNDONE: Not required for ARMv6-M encodings.
        //int             index = TRUE;
        //int             add = TRUE;
        //int             wback = FALSE;        
        uint32_t        offsetAddress;
        uint32_t        address;

        // UNDONE: Not required for ARMv6-m encodings.
        //if (add)
            offsetAddress = getReg(pContext, n) + imm32;
        //else
        //    offsetAddress = getReg(pContext, n) - imm32;
        //if (index)
            address = offsetAddress;
        //else
        //    address = getReg(pContext, n);
        unalignedMemWrite(pContext, address, 4, getReg(pContext, t));
        //if (wback)
        //    setReg(pContext, n, offsetAddress);
    }

    return PINKYSIM_STEP_OK;
}

static int ldrImmediateT2(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t        t = (instr & (0x7 << 8)) >> 8;
        uint32_t        n = SP;
        uint32_t        imm32 = (instr & 0xFF) << 2;
        // UNDONE: Not required for ARMv6-M encodings.
        //int             index = TRUE;
        //int             add = TRUE;
        //int             wback = FALSE;        
        uint32_t        offsetAddress;
        uint32_t        address;

        // UNDONE: Not required for ARMv6-m encodings.
        //if (add)
            offsetAddress = getReg(pContext, n) + imm32;
        //else
        //    offsetAddress = getReg(pContext, n) - imm32;
        //if (index)
            address = offsetAddress;
        //else
        //    address = getReg(pContext, n);
        setReg(pContext, t, unalignedMemRead(pContext, address, 4));
        //if (wback)
        //    setReg(pContext, n, offsetAddress);
    }

    return PINKYSIM_STEP_OK;
}

static int adr(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t d = (instr & (0x7 << 8)) >> 8;
        uint32_t imm32 = (instr & 0xFF) << 2;
        // UNDONE: Add is forced to be TRUE in the ARMv6-m encoding.
        //int      add = TRUE;
        uint32_t result;

        // UNDONE: Add is forced to be TRUE in the ARMv6-m encoding.
        //if (add)
            result = Align(getReg(pContext, PC), 4) + imm32;
        //else
        //    result = Align(getReg(pContext, PC), 4) - imm32;
        setReg(pContext, d, result);
    }

    return PINKYSIM_STEP_OK;
}

static int addSPT1(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t        d = (instr & (0x7 << 8)) >> 8;
        uint32_t        imm32 = (instr & 0xFF) << 2;
        // UNDONE: Not used in ARMv6-m encodings.
        //int             setFlags = FALSE;
        AddResults      addResults;
        
        addResults = AddWithCarry(getReg(pContext, SP), imm32, 0);
        setReg(pContext, d, addResults.result);
    }

    return PINKYSIM_STEP_OK;
}


static int misc16BitInstructions(PinkySimContext* pContext, uint16_t instr)
{
    int result = PINKYSIM_STEP_UNDEFINED;
    
    if ((instr & 0x0F80) == 0x0000)
        result = addSPT2(pContext, instr);
    else if ((instr & 0x0F80) == 0x0080)
        result = subSP(pContext, instr);
    else if ((instr & 0x0FC0) == 0x0200)
        result = sxth(pContext, instr);
    else if ((instr & 0x0FC0) == 0x0240)
        result = sxtb(pContext, instr);
    else if ((instr & 0x0FC0) == 0x0280)
        result = uxth(pContext, instr);
    else if ((instr & 0x0FC0) == 0x02C0)
        result = uxtb(pContext, instr);
    else if ((instr & 0x0E00) == 0x0400)
        result = push(pContext, instr);
    else if ((instr & 0x0FE0) == 0x0660)
        result = cps(pContext, instr);
    else if ((instr & 0x0FC0) == 0x0A00)
        result = rev(pContext, instr);
        
    return result;
}

static int addSPT2(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t        d = 13;
        uint32_t        imm32 = (instr & 0x7F) << 2;
        // UNDONE: Not used in ARMv6-m encodings.
        //int             setFlags = FALSE;
        AddResults      addResults;
        
        addResults = AddWithCarry(getReg(pContext, SP), imm32, 0);
        setReg(pContext, d, addResults.result);
    }

    return PINKYSIM_STEP_OK;
}

static int subSP(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t        d = 13;
        uint32_t        imm32 = (instr & 0x7F) << 2;
        // UNDONE: Not used in ARMv6-m encodings.
        //int             setFlags = FALSE;
        AddResults      addResults;
        
        addResults = AddWithCarry(getReg(pContext, SP), ~imm32, 1);
        setReg(pContext, d, addResults.result);
    }

    return PINKYSIM_STEP_OK;
}

static int sxth(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t        d = instr & 0x7;
        uint32_t        m = (instr & (0x7 << 3)) >> 3;
        uint32_t        rotation = 0;
        uint32_t        rotated;

        // UNDONE: This rotation is hardcoded as 0 for ARMv6-m.
        rotated = ROR(getReg(pContext, m), rotation);
        setReg(pContext, d, signExtend16(rotated));
    }

    return PINKYSIM_STEP_OK;
}

static uint32_t ROR(uint32_t x, uint32_t shift)
{
    ShiftResults results = {0, 0};
    
    assert (shift >= 0);
    
    if (shift == 0)
        results.result = x;
    else
        results = ROR_C(x, shift);
    return results.result;
}

static int sxtb(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t        d = instr & 0x7;
        uint32_t        m = (instr & (0x7 << 3)) >> 3;
        uint32_t        rotation = 0;
        uint32_t        rotated;

        // UNDONE: This rotation is hardcoded as 0 for ARMv6-m.
        rotated = ROR(getReg(pContext, m), rotation);
        setReg(pContext, d, signExtend8(rotated));
    }

    return PINKYSIM_STEP_OK;
}

static int uxth(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t        d = instr & 0x7;
        uint32_t        m = (instr & (0x7 << 3)) >> 3;
        uint32_t        rotation = 0;
        uint32_t        rotated;

        // UNDONE: This rotation is hardcoded as 0 for ARMv6-m.
        rotated = ROR(getReg(pContext, m), rotation);
        setReg(pContext, d, zeroExtend16(rotated));
    }

    return PINKYSIM_STEP_OK;
}

static int uxtb(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t        d = instr & 0x7;
        uint32_t        m = (instr & (0x7 << 3)) >> 3;
        uint32_t        rotation = 0;
        uint32_t        rotated;

        // UNDONE: This rotation is hardcoded as 0 for ARMv6-m.
        rotated = ROR(getReg(pContext, m), rotation);
        setReg(pContext, d, zeroExtend8(rotated));
    }

    return PINKYSIM_STEP_OK;
}

static int push(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t    registers = ((instr & (1 << 8)) << 6) | (instr & 0xFF);
        uint32_t    address;
        int         i;
        
        if (bitCount(registers) < 1)
            return PINKYSIM_STEP_UNPREDICTABLE;
        
        address = getReg(pContext, SP) - 4 * bitCount(registers);
        for (i = 0 ; i <= 14 ; i++)
        {
            if (registers & (1 << i))
            {
                alignedMemWrite(pContext, address, 4, getReg(pContext, i));
                address += 4;
            }
        }
        setReg(pContext, SP, getReg(pContext, SP) - 4 * bitCount(registers));
    }

    return PINKYSIM_STEP_OK;
}

static uint32_t bitCount(uint32_t value)
{
    uint32_t count = 0;
    
    while (value)
    {
        value = value & (value - 1);
        count++;
    }
    return count;
}

static int cps(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t im = instr & (1 << 4);
        
        if ((instr & 0xF) != 0x2)
            return PINKYSIM_STEP_UNPREDICTABLE;
        
        if (currentModeIsPrivileged(pContext))
        {
            if (im)
                pContext->PRIMASK |= PRIMASK_PM;
            else
                pContext->PRIMASK &= ~PRIMASK_PM;
        }

    }

    return PINKYSIM_STEP_OK;
}

static int currentModeIsPrivileged(PinkySimContext* pContext)
{
    // This simulator only supports privileged mode.
    return TRUE;
}

static int rev(PinkySimContext* pContext, uint16_t instr)
{
    if (ConditionPassedForNonBranchInstr(pContext))
    {
        uint32_t        d = instr & 0x7;
        uint32_t        m = (instr & (0x7 << 3)) >> 3;
        uint32_t        value;
        uint32_t        result;
        
        value = getReg(pContext, m);
        result = (value << 24) | (value >> 24) | ((value & 0xFF00) << 8) | ((value & 0xFF0000) >> 8);
        setReg(pContext, d, result);
    }

    return PINKYSIM_STEP_OK;
}

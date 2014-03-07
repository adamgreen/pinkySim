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


/* Function Prototypes */
static int shiftAddSubtractMoveCompare(PinkySimContext* pContext, uint16_t instr);
static int lslImmediate(PinkySimContext* pContext, uint16_t instr);
static int ConditionPassedForNonBranchInstr(const PinkySimContext* pContext);
static int InITBlock(const PinkySimContext* pContext);
static DecodedImmShift DecodeImmShift(uint32_t typeBits, uint32_t imm5);
static ShiftResults Shift_C(uint32_t value, SRType type, uint32_t amount, uint32_t carry_in);
static ShiftResults LSL_C(uint32_t x, uint32_t shift);
static uint32_t getReg(const PinkySimContext* pContext, uint32_t reg);
static void setReg(PinkySimContext* pContext, uint32_t reg, uint32_t value);


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
    if ((instr & 0xD000) == 0x0000)
        return lslImmediate(pContext, instr);
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
        ShiftResults  shiftResults;

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
    DecodedImmShift results = {SRType_LSL, imm5};
    return results;
}

static ShiftResults Shift_C(uint32_t value, SRType type, uint32_t amount, uint32_t carry_in)
{
    ShiftResults results;

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
        case SRType_ASR:
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
    
    assert (shift > 0 && shift < 32);

    results.carryOut = (x & (1 << (32 - shift)));
    results.result = x << shift;
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

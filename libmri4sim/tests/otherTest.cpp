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
/* NOTE: These are tests for Platform_* APIs which must be included in libmrisim.a for things to link but the
         functionality won't be called from MRI core because of the code paths which end up being enabled. */
#include "mri4simBaseTest.h"
extern "C"
{
    #include <platforms.h>
}

TEST_GROUP_BASE(otherTests, mri4simBase)
{
    void setup()
    {
        mri4simBase::setup();
    }

    void teardown()
    {
        mri4simBase::teardown();
    }
};


TEST(otherTests, CallAPIsNotUsedByMRI_GuaranteeCoverage)
{
    Platform_CommClearInterrupt();
    Platform_CommPrepareToWaitForGdbConnection();
    CHECK_FALSE(Platform_CommIsWaitingForGdbToConnect());
    Platform_CommWaitForReceiveDataToStop();
}

TEST(otherTests, AdvanceProgramCounterToNextInstruction_WithInvalidPC_ShouldNotModifyPC)
{
    m_pContext->pc = 0xFFFFFFF0;
        Platform_AdvanceProgramCounterToNextInstruction();
    CHECK_EQUAL(0xFFFFFFF0, m_pContext->pc);
}

TEST(otherTests, AdvanceProgramCounterToNextInstruction_AdvancePast32BitInstruction)
{
    /* This is a 32-bit BL instruction. */
    emitInstruction32("11110Siiiiiiiiii", "11j1kiiiiiiiiiii", 0, 0, 1, 1, 0);
        Platform_AdvanceProgramCounterToNextInstruction();
    CHECK_EQUAL(INITIAL_PC + 4, m_pContext->pc);
}

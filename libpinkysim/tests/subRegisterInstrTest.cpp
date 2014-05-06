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

#include "pinkySimBaseTest.h"

TEST_GROUP_BASE(subRegister, pinkySimBase)
{
    void setup()
    {
        pinkySimBase::setup();
    }

    void teardown()
    {
        pinkySimBase::teardown();
    }
};


/* SUB - Register
   Encoding: 000 11 0 1 Rm:3 Rn:3 Rd:3 */
TEST(subRegister, UseLowestRegisterForAllArgs)
{
    emitInstruction16("0001101mmmnnnddd", R0, R0, R0);
    setExpectedXPSRflags("nZCv");
    setExpectedRegisterValue(R0, 0);
    pinkySimStep(&m_context);
}

TEST(subRegister, UseHigestRegisterForAllArgs)
{
    emitInstruction16("0001101mmmnnnddd", R7, R7, R7);
    setExpectedXPSRflags("nZCv");
    setExpectedRegisterValue(R7, 0);
    pinkySimStep(&m_context);
}

TEST(subRegister, UseDifferentRegistersForEachArg)
{
    emitInstruction16("0001101mmmnnnddd", R1, R2, R0);
    setExpectedXPSRflags("nzCv");
    setExpectedRegisterValue(R0, 0x22222222U - 0x11111111U);
    pinkySimStep(&m_context);
}

// Force APSR flags to be set which haven't already been covered above.
TEST(subRegister, ForceCarryClearToIndicateBorrowAndResultWillBeNegative)
{
    emitInstruction16("0001101mmmnnnddd", R1, R0, R2);
    setExpectedXPSRflags("Nzcv");
    setRegisterValue(R1, 1);
    setExpectedRegisterValue(R2, 0U - 1U);
    pinkySimStep(&m_context);
}

TEST(subRegister, ForceNegativeOverflow)
{
    emitInstruction16("0001101mmmnnnddd", R1, R2, R0);
    setExpectedXPSRflags("nzCV");
    setRegisterValue(R2, 0x80000000U);
    setRegisterValue(R1, 1U);
    setExpectedRegisterValue(R0, (int32_t)0x80000000U - 1);
    pinkySimStep(&m_context);
}

TEST(subRegister, ForcePositiveOverflow)
{
    emitInstruction16("0001101mmmnnnddd", R1, R2, R0);
    setExpectedXPSRflags("NzcV");
    setRegisterValue(R2, 0x7FFFFFFFU);
    setRegisterValue(R1, -1U);
    setExpectedRegisterValue(R0, 0x7FFFFFFFU + 1U);
    pinkySimStep(&m_context);
}
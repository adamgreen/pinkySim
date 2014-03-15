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

TEST_GROUP_BASE(movRegister, pinkySimBase)
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


/* MOV - Register Encoding 1
   Encoding: 010001 10 D:1 Rm:4 Rd:3 
   NOTE: This encoding doesn't update the APSR flags. */
TEST(movRegister, UseLowestRegisterForAllArgs)
{
    emitInstruction16("01000110dmmmmddd", R0, R0);
    pinkySimStep(&m_context);
}

TEST(movRegister, UseHighRegisterForAllArgs)
{
    emitInstruction16("01000110dmmmmddd", LR, LR);
    pinkySimStep(&m_context);
}

TEST(movRegister, MoveHighRegisterToLowRegister)
{
    emitInstruction16("01000110dmmmmddd", R7, R12);
    setExpectedRegisterValue(R7, 0xCCCCCCCC);
    pinkySimStep(&m_context);
}

TEST(movRegister, MoveLowRegisterToLHighRegister)
{
    emitInstruction16("01000110dmmmmddd", R12, R7);
    setExpectedRegisterValue(R12, 0x77777777);
    pinkySimStep(&m_context);
}

TEST(movRegister, MoveOddAddressIntoPCAndMakeSureLSbitIsCleared)
{
    emitInstruction16("01000110dmmmmddd", PC, R1);
    setExpectedRegisterValue(PC, 0x11111110);
    pinkySimStep(&m_context);
}

TEST(movRegister, MoveEvenAddressIntoPC)
{
    emitInstruction16("01000110dmmmmddd", PC, R2);
    setExpectedRegisterValue(PC, 0x22222222);
    pinkySimStep(&m_context);
}

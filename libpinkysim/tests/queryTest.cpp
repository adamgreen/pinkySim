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
#include <signal.h>
#include <mri.h>
#include "mri4simBaseTest.h"

TEST_GROUP_BASE(queryTests, mri4simBase)
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


TEST(queryTests, qSupported_ReturnsExpectedOptionsAndCorrectPacketSizeOf16k)
{
    mockIComm_InitReceiveChecksummedData("+$qSupported#", "+$c#");
        mri4simRun(mockIComm_Get(), TRUE);
    appendExpectedTPacket(SIGTRAP, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC);
    appendExpectedString("+$qXfer:memory-map:read+;qXfer:features:read+;PacketSize=4000#+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
}

TEST(queryTests, qXfer_TargetXML_ReturnsExpectedOutputForCortexM0)
{
    mockIComm_InitReceiveChecksummedData("+$qXfer:features:read:target.xml:0,65536#", "+$c#");
        mri4simRun(mockIComm_Get(), TRUE);
    appendExpectedTPacket(SIGTRAP, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC);
    appendExpectedString("+$l<?xml version=\"1.0\"?>\n"
                         "<!DOCTYPE feature SYSTEM \"gdb-target.dtd\">\n"
                         "<target>\n"
                         "<feature name=\"org.gnu.gdb.arm.m-profile\">\n"
                         "<reg name=\"r0\" bitsize=\"32\"/>\n"
                         "<reg name=\"r1\" bitsize=\"32\"/>\n"
                         "<reg name=\"r2\" bitsize=\"32\"/>\n"
                         "<reg name=\"r3\" bitsize=\"32\"/>\n"
                         "<reg name=\"r4\" bitsize=\"32\"/>\n"
                         "<reg name=\"r5\" bitsize=\"32\"/>\n"
                         "<reg name=\"r6\" bitsize=\"32\"/>\n"
                         "<reg name=\"r7\" bitsize=\"32\"/>\n"
                         "<reg name=\"r8\" bitsize=\"32\"/>\n"
                         "<reg name=\"r9\" bitsize=\"32\"/>\n"
                         "<reg name=\"r10\" bitsize=\"32\"/>\n"
                         "<reg name=\"r11\" bitsize=\"32\"/>\n"
                         "<reg name=\"r12\" bitsize=\"32\"/>\n"
                         "<reg name=\"sp\" bitsize=\"32\" type=\"data_ptr\"/>\n"
                         "<reg name=\"lr\" bitsize=\"32\"/>\n"
                         "<reg name=\"pc\" bitsize=\"32\" type=\"code_ptr\"/>\n"
                         "<reg name=\"xpsr\" bitsize=\"32\" regnum=\"25\"/>\n"
                         "</feature>\n"
                         "</target>\n#+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
}

TEST(queryTests, qXfer_MemoryMap_ReturnsTwoRegions)
{
    mockIComm_InitReceiveChecksummedData("+$qXfer:memory-map:read::0,65536#", "+$c#");
        mri4simRun(mockIComm_Get(), TRUE);
    appendExpectedTPacket(SIGTRAP, 0, INITIAL_SP, INITIAL_LR, INITIAL_PC);
    appendExpectedString("+$l"
                         "<?xml version=\"1.0\"?>"
                         "<!DOCTYPE memory-map PUBLIC \"+//IDN gnu.org//DTD GDB Memory Map V1.0//EN\" \"http://sourceware.org/gdb/gdb-memory-map.dtd\">"
                         "<memory-map>"
                         "<memory type=\"flash\" start=\"0x0\" length=\"0x8\"> <property name=\"blocksize\">1</property></memory>"
                         "<memory type=\"ram\" start=\"0x10000000\" length=\"0x8000\"></memory>"
                         "</memory-map>"
                         "#+");
    STRCMP_EQUAL(checksumExpected(), mockIComm_GetTransmittedData());
}
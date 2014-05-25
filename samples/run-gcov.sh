#! /usr/bin/env bash
# Copyright 2014 Adam Green (http://mbed.org/users/AdamGreen/)
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

displayUsage() {
    echo Usage:   run-gcov SampleName
    echo Example: run-gcov FileTest
    exit 1
}

# Check command line parameters.
if [ "$1" == "" ] ; then
    displayUsage
fi

ARM_GCOV=arm-none-eabi-gcov
SAMPLE_NAME=$1
TEXT_OUT=${SAMPLE_NAME}_output.txt
GCOV_RESULTS=gcov/$SAMPLE_NAME
SAMPLE_OBJDIR=gcov/obj/$SAMPLE_NAME
LIBSTARTUP_OBJDIR=gcov/obj/libstartup

# Find sample and libstartup object files.
pushd $SAMPLE_OBJDIR >/dev/null 2>&1
SAMPLE_OBJECTS=`ls *.o`
popd  >/dev/null 2>&1
pushd $LIBSTARTUP_OBJDIR  >/dev/null 2>&1
LIBSTARTUP_OBJECTS=`ls *.o`
popd  >/dev/null 2>&1

rm $TEXT_OUT >/dev/null 2>&1
mkdir -p $GCOV_RESULTS >/dev/null 2>&1

# Iterate through each object and extract code coverage results.
for obj in $SAMPLE_OBJECTS ; do
    $ARM_GCOV --object-directory=$SAMPLE_OBJDIR $obj >>$TEXT_OUT
done
for obj in $LIBSTARTUP_OBJECTS ; do
    $ARM_GCOV --object-directory=$LIBSTARTUP_OBJDIR $obj >>$TEXT_OUT
done

# Get final output.
mv $TEXT_OUT $GCOV_RESULTS
mv *.gcov $GCOV_RESULTS
../mri/CppUTest/scripts/filterGcov.sh $GCOV_RESULTS/$TEXT_OUT /dev/null $GCOV_RESULTS/$TEXT_OUT
cat $GCOV_RESULTS/$TEXT_OUT
echo Detailed code coverage results can be found in $GCOV_RESULTS

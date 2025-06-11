#!/usr/bin/env bash
#
#       Copyright (c) 2018 German Aerospace Center DLR e.V. (GSOC)
#
#       Licensed under the Apache License, Version 2.0 (the "License");
#       you may not use this file except in compliance with the License.
#       You may obtain a copy of the License at
#
#               http://www.apache.org/licenses/LICENSE-2.0
#
#       Unless required by applicable law or agreed to in writing, software
#       distributed under the License is distributed on an "AS IS" BASIS,
#       WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#       See the License for the specific language governing permissions and
#       limitations under the License.
#
#       This file is part of the openvocs project. http://openvocs.org
#
#       ------------------------------------------------------------------------
#
#       File            testrun_runner.sh
#       Authors         Markus Toepfer
#       Date            2018-11-05
#
#       Project         ov_example_app_echo
#
#       Description     Run each TEST.test of a folder and log Ok or NOK
#                       for each executed testfile of the folder.
#
#                       EXAMPLE OUTPUT
#
#                       [OK]  1/5 filename1.test
#                       [NOK] 2/5 filename2.test
#
#                       MODES
#
#                       (1) RUN ALL TESTS (log but ignore errors)
#                           use script with 2 parameters
#                           e.g. ./testrun_runner.sh logfile /path
#
#                           This mode will not return a test failure and
#                           may be used to run all tests and return success
#                           if all tests was run. (test results are logged)
#
#                       (2) FAIL ON ERROR (Fail on first error)
#                           use script with 3 parameters
#                           e.g. ./testrun_runner.sh logfile /path 1
#
#                           This mode returns -1 on the first test failure.
#
#                       PARAMETER
#
#                       (1) path to logfile destination
#                       (2) path to folder with test cases
#
#
#       Usage           ./testrun_runner.sh /path/to/logfile /path/to/test/dir
#
#       Dependencies    bash, tail, ls, grep, wc
#
#       Last changed    2018-11-05
#       ------------------------------------------------------------------------

if [ -z $1 ]; then
        echo "ERROR ... NO LOGFILE INPUT TO SRCIPT"
        exit 1
fi
LOGFILE=$1

if [ -z $2 ]; then
        echo "ERROR ... FOLDER INPUT TO SRCIPT"
        exit 1
fi
FOLDER=$2

FAIL_ON_ERROR=0
if [ ! -z $3 ]; then
        FAIL_ON_ERROR=1
fi

if [ ! -w $LOGFILE ]; then
        echo "ERROR ... LOGFILE NOT WRITABLE"
        exit 1
fi

#       ------------------------------------------------------------------------
#       PERFORM TESTRUN
#       ------------------------------------------------------------------------

FILES=`ls  $FOLDER/ | grep "\.test" | wc -l`
if [ $? -ne 0 ]; then
        echo "ERROR ... could not count files of $FOLDER"
        exit 1
fi
c=0

if [ $FILES -eq 0 ]; then
        exit 0
fi

for i in $FOLDER/*.test
do
        c=$((c+1))

        # RUN EXECUTABLE
        $i 2>&1 >> $LOGFILE

        # CHECK RETURN OF EXECUTABLE
        if [ $? -ne 0 ]; then

                echo "NOK\t("$c"/"$FILES")\t"$i

                if [ $FAIL_ON_ERROR -eq 1 ]; then
                        exit 1
                fi
        else
                echo "OK\t("$c"/"$FILES")\t"$i
        fi
done
exit 0

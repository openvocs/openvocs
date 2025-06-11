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
#       Date            2020-01-24
#
#       This is a very basic testrunner, running all tests within a given
#       subfolder of build/test
#
#       ------------------------------------------------------------------------

FOLDER=$OPENVOCS_ROOT"/build/test"

if [ -z $1 ]; then

    echo "You may specific a specific module using:"
    echo "./testrunner <ov_modulue_name>"

else

    if [ $1 == "-h" ]; then
        echo "Usage:"
        echo "1) to run all tests: ./testrunner"
        echo "2) to run module tests: ./testrunner <module name>"
        exit 0
    else
        FOLDER+="/"$1

    fi

fi

#       ------------------------------------------------------------------------
#       PERFORM TESTRUN | BREAK ON ERROR
#       ------------------------------------------------------------------------

SOURCES=`find $FOLDER -name "*.run"`
if [ $? -ne 0 ]; then
        echo "ERROR ... could find source of $FOLDER"
        exit 1
fi

FILES=0
for i in $SOURCES
do
        FILES=$((FILES+1))
done

if [ `uname` == "Darwin" ]; then
    OV_VALGRIND=
fi

OV_VALGRIND=

for i in $SOURCES
do
        c=$((c+1))

        if [ "X$OPENVOCS_TEST_SPARSE_OUT" == "X" ]; then

            $OV_VALGRIND $i 2>&1

            # CHECK RETURN OF EXECUTABLE
            if [ $? -ne 0 ]; then

                echo
                echo ================================================================================
                echo "NOK ("$c"/"$FILES") "$i
                exit 1
            else

                echo
                echo ================================================================================
                echo "OK ("$c"/"$FILES") "$i
            fi

        else

            $OV_VALGRIND $i 1>/dev/null

            # CHECK RETURN OF EXECUTABLE
            if [ $? -ne 0 ]; then

                echo
                echo ================================================================================
                echo "NOK ("$c"/"$FILES") "$i
                exit 1
            else

                echo "OK ("$c"/"$FILES") "$i
            fi


        fi
done
exit 0

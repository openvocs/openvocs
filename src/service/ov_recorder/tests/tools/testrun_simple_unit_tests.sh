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
#       File            testrun_simple_unit_tests.sh
#       Authors         Markus Toepfer
#       Date            2018-11-05
#
#       Project         ov_example_app_echo
#
#       Description     Run all test executables [PATH_TESTS]/*.test
#                       Run the whole folder, until an error occurs.
#
#                       MODE         FAIL ON ERROR (Fail on first test error)
#
#                       LOGFILE      [PATH_LOGFILE]/unit.<time>.log
#
#
#       Usage           ./testrun_simple_unit_tests.sh /path/to/project
#
#       Dependencies    bash, touch, chmod, ls, wc, date
#
#       Last changed    2018-11-05
#       ------------------------------------------------------------------------

TEST_TYPE="unit"
FOLDER_LOGFILE="build/tests/log"
FOLDER_TESTS="build/tests/unit"
RUNNER_SCRIPT="./tests/tools/testrun_runner.sh"

echo "-------------------------------------------------------"
echo "               SIMPLE $TEST_TYPE TESTING"
echo "-------------------------------------------------------"

start_time=$(date "+%Y.%m.%d-%H.%M.%S.%N")

# SET A LOGFILE
LOGFILE="$FOLDER_LOGFILE/$TEST_TYPE.$start_time.log"
echo " (log)   $start_time" > $LOGFILE
touch $LOGFILE
chmod a+w $LOGFILE

echo "-------------------------------------------------------" >> $LOGFILE
echo "               REPORT $TEST_TYPE TESTING"                >> $LOGFILE
echo "-------------------------------------------------------" >> $LOGFILE

# RUN THE RUNNER
sh $RUNNER_SCRIPT $LOGFILE $FOLDER_TESTS FAIL_ON_ERROR
RESULT=$?

end_time=$(date "+%Y.%m.%d-%H.%M.%S.%N")

# FINISH THE REPORT
echo "-------------------------------------------------------" >> $LOGFILE
echo "DONE \t $TEST_TYPE TEST RUN" >> $LOGFILE
if [ $RESULT -eq 0 ]; then
        echo "RESULT\t SUCCESS" >> $LOGFILE
else
        echo "RESULT\t FAILURE" >> $LOGFILE
fi
echo "START \t $start_time" >> $LOGFILE
echo "END   \t $end_time" >> $LOGFILE
echo "-------------------------------------------------------" >> $LOGFILE

# DUMP THE REPORT ON SUCCESS
if [ $RESULT -eq 0 ]; then
        cat $LOGFILE
        echo ""
else
        echo ""
        echo "$TEST_TYPE TEST FAILED"
        echo "Logfile dump stopped to point to last error."
fi
exit $RESULT

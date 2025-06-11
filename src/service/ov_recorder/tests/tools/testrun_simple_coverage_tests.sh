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
#       File            testrun_simple_coverage_tests.sh
#       Authors         Markus Toepfer
#       Date            2018-11-05
#
#       Project         ov_example_app_echo
#
#       Description     Count functions of folder src vs unit test functions.
#
#                       CONVENTION
#
#                       Each (namespaced) function in a *.c source file will have 
#                       a corresponding test function with a prefix "test_" in 
#                       some testfile with a suffix "*_test.c"
#
#                       EXAMPLE      function | test_function
#
#                       NOTE         This coverage test just covers the
#                                    observance of the given coding convention.
#
#       Usage           ./testrun_simple_coverage_tests.sh /path/to/project
#
#       Dependencies    bash, ctags, awk, sed, grep
#
#       Last changed    2018-11-29
#       ------------------------------------------------------------------------

start_time=$(date "+%Y.%m.%d-%H.%M.%S.%N")
NAMESPACE_PREFIX="ov_"
INTERFACE_PREFIX="impl_"
TEST_PREFIX="test_"
ROOT_DIR=$1
FOLDER_LOGFILE="$ROOT_DIR/build/tests/log"
# SET A LOGFILE
LOGFILE="$FOLDER_LOGFILE/coverage.$start_time.log"
touch $LOGFILE
chmod a+w $LOGFILE

echo "-------------------------------------------------------" >> $LOGFILE
echo "               REPORT COVERAGE TESTING"                  >> $LOGFILE
echo "-------------------------------------------------------" >> $LOGFILE
echo "   TIME    $start_time" >> $LOGFILE
echo "" >> $LOGFILE

cd $ROOT_DIR
if [ $? -ne 0 ]; then
        exit 1
fi
# GENERATE CTAGS SOURCE
ctags --c-types=f -R
# remove the ctags stuff, just make use of the function lines
sed -e '/[ ]*m$/d' -e '/TAG/d' <tags>tags_coverage_functions_only
# remove anything but the function names
awk '{print $1 }' tags_coverage_functions_only > tags_coverage_function_all

# CUSTOM PREFIXING TO BE DONE HERE

# functions with custom PREFIX (external namespace)
# (e.g. "ov_")
cat tags_coverage_function_all | \
        sed -ne '/^'$NAMESPACE_PREFIX'.*/p' \
        > tags_functions_to_test_prep

# functions with interface implementations 
# (e.g. "impl_")
cat tags_coverage_function_all | \
        sed -ne '/^'$INTERFACE_PREFIX'.*/p' \
        >> tags_functions_to_test_prep

# remove all interface implementation prefixes, 
# which fit a defined interface test
cat tags_coverage_function_all | \
        sed -ne '/^'$TEST_PREFIX$INTERFACE_PREFIX'.*/p' |\
        sed 's/^'$TEST_PREFIX$INTERFACE_PREFIX'*//g' \
        > tags_functions_names_interface_tests

# remove all interface tests from tags_functions_to_test
grep -Fvf tags_functions_names_interface_tests tags_functions_to_test_prep \
        > tags_functions_to_test

# identify test functions
cat tags_coverage_function_all | \
        sed -ne '/^'$TEST_PREFIX'.*/p' \
        > tags_functions_test_implementations

# Found functions:
while read line;
do
        grep -n '^'$TEST_PREFIX$line'$' tags_functions_test_implementations > \
        /dev/null || echo $line >> tags_functions_untested
done < tags_functions_to_test

sCount="$(cat tags_coverage_function_all | grep -i ^$NAMESPACE_PREFIX | wc -l)"
tCount="$(cat tags_coverage_function_all | grep -i ^$TEST_PREFIX | wc -l)"
iCount="$(cat tags_functions_names_interface_tests | wc -l)"

echo "
   UNTESTED\n" >> $LOGFILE

if [ -e tags_functions_untested ]; then
        cat tags_functions_untested >> $LOGFILE
        uCount="$(cat tags_functions_untested | wc -l )"
else
        uCount=0
fi

echo "---------------------------------------------------------\n" >> $LOGFILE
echo "   count sources to test\t $sCount"  >> $LOGFILE
echo "   count tests for sources\t $tCount"  >> $LOGFILE
echo "   count interface test units\t $iCount" >> $LOGFILE
echo "   count untested sources\t $uCount" >> $LOGFILE

echo  >> $LOGFILE
echo "$sCount $iCount $uCount $tCount" | \
awk '{ printf "   test coverage\t\t %.2f %% \n", $4/($1 + $2 + $3) * 100}' >> $LOGFILE

if [ $iCount != 0 ]; then
        echo "$tCount $iCount" | \
        awk '{ printf "   interface based\t\t %.2f %% \n", $2/$1*100}' >> $LOGFILE
fi

check=$(( $sCount + $iCount - $tCount));

if [ "$check" -ne "$uCount" ]; then

       echo >> $LOGFILE
       echo "   (1) You may miss an interface tests." \
               >> $LOGFILE
       echo "   This would be visible in the UNTESTED functions list.\n" \
               >> $LOGFILE
       echo "   (2) You may have a functions e.g. prefix_count, as well as" \
               >> $LOGFILE
       echo "   similar named interfaces e.g. impl_count | test_impl_count" \
               >> $LOGFILE
       echo "   and miss the test function test_prefix_count.\n" \
               >> $LOGFILE
       echo "   Please check UNTESTED, as well as all functions, which" \
               >> $LOGFILE
       echo "   contain one of the following strings: (may be empty)\n" \
               >> $LOGFILE

       cat tags_coverage_function_all | \
               sed -ne '/^'$TEST_PREFIX$INTERFACE_PREFIX'.*/p'|\
               sed 's/^'$TEST_PREFIX$INTERFACE_PREFIX'*//g' >> $LOGFILE

fi

echo "\nCOVERAGE IS BASED ON COUNTING PRESENCE OF FUNCTION NAMES" >> $LOGFILE
echo "---------------------------------------------------------\n" >> $LOGFILE
cat $LOGFILE
echo

# prevent error output if all functions are tested
echo  >> tags_functions_untested

# file cleanup
rm tags_coverage_functions_only
rm tags_coverage_function_all
rm tags_functions_to_test_prep
rm tags_functions_to_test
rm tags_functions_names_interface_tests
rm tags_functions_test_implementations
rm tags_functions_untested

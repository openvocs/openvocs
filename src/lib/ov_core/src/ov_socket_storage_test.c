/***
        ------------------------------------------------------------------------

        Copyright (c) 2026 German Aerospace Center DLR e.V. (GSOC)

        Licensed under the Apache License, Version 2.0 (the "License");
        you may not use this file except in compliance with the License.
        You may obtain a copy of the License at

                http://www.apache.org/licenses/LICENSE-2.0

        Unless required by applicable law or agreed to in writing, software
        distributed under the License is distributed on an "AS IS" BASIS,
        WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
        See the License for the specific language governing permissions and
        limitations under the License.

        This file is part of the openvocs project. https://openvocs.org

        ------------------------------------------------------------------------
*//**
        @file           ov_socket_storage_test.c
        @author         Töpfer, Markus

        @date           2026-04-08


        ------------------------------------------------------------------------
*/
#include <ov_test/testrun.h>
#include "ov_socket_storage.c"

#include <ov_base/ov_string.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_socket_storage_create(){
    
    ov_socket_storage *self = ov_socket_storage_create();
    testrun(self);
    testrun(self->sockets);
    testrun(NULL == ov_socket_storage_free(self));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_socket_storage_free(){
    
    ov_socket_storage *self = ov_socket_storage_create();
    testrun(self);
    
    ov_json_value *one = ov_socket_storage_get(self, 1);
    testrun(one);
    testrun(ov_json_object_is_empty(one));

    ov_json_value *two = ov_socket_storage_get(self, 2);
    testrun(two);
    testrun(ov_json_object_is_empty(two));

    testrun(NULL == ov_socket_storage_free(self));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_socket_storage_get(){
    
    ov_socket_storage *self = ov_socket_storage_create();
    testrun(self);
    
    ov_json_value *one = ov_socket_storage_get(self, 1);
    testrun(one);
    testrun(ov_json_object_is_empty(one));
    testrun(one == ov_socket_storage_get(self, 1));

    ov_json_object_set(one, "key", ov_json_string("val"));

    ov_json_value *two = ov_socket_storage_get(self, 2);
    testrun(two);
    testrun(two != one);

    one = ov_socket_storage_get(self, 1);
    testrun(!ov_json_object_is_empty(one));

    testrun(NULL == ov_socket_storage_free(self));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_socket_storage_drop(){
    
    ov_socket_storage *self = ov_socket_storage_create();
    testrun(self);
    
    ov_json_value *one = ov_socket_storage_get(self, 1);
    testrun(one);
    testrun(ov_json_object_is_empty(one));
    testrun(one == ov_socket_storage_get(self, 1));

    testrun(ov_socket_storage_drop(self, 1));
    testrun(ov_dict_is_empty(self->sockets));
    testrun(one != ov_socket_storage_get(self, 1));

    testrun(NULL == ov_socket_storage_free(self));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CLUSTER                                                    #CLUSTER
 *
 *      ------------------------------------------------------------------------
 */

int all_tests() {

    testrun_init();
    testrun_test(test_ov_socket_storage_create);
    testrun_test(test_ov_socket_storage_free);
    testrun_test(test_ov_socket_storage_get);
    testrun_test(test_ov_socket_storage_drop);

    return testrun_counter;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST EXECUTION                                                  #EXEC
 *
 *      ------------------------------------------------------------------------
 */

testrun_run(all_tests);

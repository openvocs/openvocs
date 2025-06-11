/***
        ------------------------------------------------------------------------

        Copyright (c) 2022 German Aerospace Center DLR e.V. (GSOC)

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

        @author         Michael J. Beer, DLR/GSOC

        ------------------------------------------------------------------------
*/

#include "ov_sip_permission.c"
#include <ov_test/ov_test.h>

/*----------------------------------------------------------------------------*/

static int ov_sip_permission_to_json_test() {

    ov_sip_permission permission = {0};

    testrun(0 == ov_sip_permission_to_json(permission));

    permission.caller = "ildico";
    testrun(0 == ov_sip_permission_to_json(permission));

    permission.loop = "etzel";
    ov_json_value *jpermission = ov_sip_permission_to_json(permission);

    testrun(0 != jpermission);

    jpermission = ov_json_value_free(jpermission);
    testrun(0 == jpermission);

    permission.loop = 0;
    testrun(0 == ov_sip_permission_to_json(permission));

    // loop not set
    // caller set
    // callee set
    permission.caller = "ildico";
    permission.callee = "stilicho";
    permission.loop = 0;
    testrun(0 == ov_sip_permission_to_json(permission));

    // loop set
    // caller set
    // callee not set
    permission.loop = "etzel";
    jpermission = ov_sip_permission_to_json(permission);

    testrun(0 != jpermission);

    jpermission = ov_json_value_free(jpermission);
    testrun(0 == jpermission);

    // loop set
    // caller set
    // callee set
    permission.loop = "etzel";
    jpermission = ov_sip_permission_to_json(permission);

    testrun(0 != jpermission);

    jpermission = ov_json_value_free(jpermission);
    testrun(0 == jpermission);

    // loop set
    // caller not set
    // callee set
    permission.caller = 0;
    jpermission = ov_sip_permission_to_json(permission);

    testrun(0 != jpermission);

    jpermission = ov_json_value_free(jpermission);
    testrun(0 == jpermission);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int ov_sip_permission_from_json_test() {

    ov_sip_permission permission = ov_sip_permission_from_json(0, 0);
    testrun(0 == permission.caller);
    testrun(0 == permission.loop);

    // caller & loop set
    ov_sip_permission ref_permission = {
        .caller = "ildico",
        .loop = "etzel",
    };

    ov_json_value *jval = ov_sip_permission_to_json(ref_permission);
    permission = ov_sip_permission_from_json(jval, 0);

    testrun(ov_sip_permission_equals(permission, ref_permission));

    // beware: pointers in permission point into the json object!
    jval = ov_json_value_free(jval);

    // callee & loop set

    ref_permission = (ov_sip_permission){
        .callee = "stilicho",
        .caller = 0,
        .loop = "etzel",
    };

    jval = ov_sip_permission_to_json(ref_permission);
    permission = ov_sip_permission_from_json(jval, 0);

    testrun(ov_sip_permission_equals(permission, ref_permission));

    // beware: pointers in permission point into the json object!
    jval = ov_json_value_free(jval);

    ref_permission = (ov_sip_permission){
        .caller = "tusnelda",
        .loop = "arminius",
        .from_epoch = 31254,
        .until_epoch = 91,
    };

    jval = ov_sip_permission_to_json(ref_permission);
    permission = ov_sip_permission_from_json(jval, 0);

    testrun(ov_sip_permission_equals(permission, ref_permission));
    jval = ov_json_value_free(jval);

    /*************************************************************************
               Missing `valid_from` and `valid_to` handled properly
     ************************************************************************/

    char const permission_without_valid[] =
        "{\"" OV_KEY_CALLER "\" : \"roger\", \"" OV_KEY_LOOP
        "\" : "
        "\"Sicilia\"}";
    jval = ov_json_value_from_string(
        permission_without_valid, strlen(permission_without_valid));
    testrun(0 != jval);

    ref_permission = (ov_sip_permission){
        .caller = "roger",
        .loop = "Sicilia",
    };

    permission = ov_sip_permission_from_json(jval, 0);
    testrun(ov_sip_permission_equals(permission, ref_permission));
    jval = ov_json_value_free(jval);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_sip_permission",
            ov_sip_permission_to_json_test,
            ov_sip_permission_from_json_test);

/*----------------------------------------------------------------------------*/

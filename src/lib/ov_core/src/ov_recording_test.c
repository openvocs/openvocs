/***
        ------------------------------------------------------------------------

        Copyright (c) 2024 German Aerospace Center DLR e.V. (GSOC)

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

#include "ov_recording.c"
#include <ov_test/ov_test.h>

static int test_ov_recording_to_json() {

    ov_recording record = {0};
    testrun(0 == ov_recording_to_json(record));

    record.end_epoch_secs = 12;
    record.start_epoch_secs = 4;

    record.id = "opladioplada";
    testrun(0 == ov_recording_to_json(record));

    record.loop = "dadada";
    testrun(0 == ov_recording_to_json(record));

    record.uri = "aha";
    ov_json_value *jval = ov_recording_to_json(record);

    testrun(0 != jval);

    jval = ov_json_value_free(jval);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_recording_from_json() {

    ov_recording rec = ov_recording_from_json(0);
    testrun(rec.id == 0);
    testrun(rec.loop == 0);
    testrun(rec.uri == 0);

    ov_recording record = {
        .end_epoch_secs = 12,
        .start_epoch_secs = 4,
        .id = "opladioplada",
        .loop = "dadada",
        .uri = "aha",

    };

    ov_json_value *jval = ov_recording_to_json(record);
    testrun(0 != jval);

    rec = ov_recording_from_json(jval);
    jval = ov_json_value_free(jval);

    testrun(rec.id != 0);
    testrun(rec.loop != 0);
    testrun(rec.uri != 0);

    testrun(0 == strcmp(rec.id, record.id));
    testrun(0 == strcmp(rec.loop, record.loop));
    testrun(0 == strcmp(rec.uri, record.uri));
    testrun(record.end_epoch_secs == rec.end_epoch_secs);
    testrun(record.start_epoch_secs == rec.start_epoch_secs);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_recording", test_ov_recording_to_json,
            test_ov_recording_from_json);

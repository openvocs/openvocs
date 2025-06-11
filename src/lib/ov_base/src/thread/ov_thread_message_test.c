/***

Copyright   2018       German Aerospace Center DLR e.V.,
                        German Space Operations Center (GSOC)

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

    This file is part of the openvocs project. http://openvocs.org

***/ /**

     \file               ov_thread_message_tests.c
     \author             Michael J. Beer, DLR/GSOC <michael.beer@dlr.de>
     \date               2017-10-29

     \ingroup            empty

 **/
/*---------------------------------------------------------------------------*/

#include "ov_thread_message.c"
#include <ov_test/ov_test.h>

/*---------------------------------------------------------------------------*/

const ov_thread_message_type INVALID =
    OV_THREAD_MESSAGE_TYPE_ENSURE_SIGNED_INT_TYPE;

/*---------------------------------------------------------------------------*/

int test_ov_thread_message_standard_create() {

    testrun(0 == ov_thread_message_standard_create(INVALID, 0));

    ov_thread_message *message = 0;
    message = ov_thread_message_standard_create(OV_GENERIC_MESSAGE, 0);

    testrun(0 != message);
    testrun(OV_THREAD_MESSAGE_MAGIC_BYTES == message->magic_bytes);
    testrun(0 == message->json_message);
    testrun(OV_GENERIC_MESSAGE == message->type);
    testrun(0 != message->free);

    message = ov_thread_message_free(message);
    testrun(0 == message);

    message = ov_thread_message_standard_create(OV_GENERIC_MESSAGE + 2, 0);

    testrun(0 != message);
    testrun(OV_THREAD_MESSAGE_MAGIC_BYTES == message->magic_bytes);
    testrun(0 == message->json_message);
    testrun(OV_GENERIC_MESSAGE + 2 == message->type);
    testrun(0 != message->free);

    message = ov_thread_message_free(message);
    testrun(0 == message);

    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

int test_ov_thread_message_free() {

    testrun(0 == ov_thread_message_free(0));

    ov_thread_message *message = 0;

    message = ov_thread_message_standard_create(OV_GENERIC_MESSAGE, 0);
    testrun(0 == ov_thread_message_free(message));

    /* Try to dispose  invalid message */
    message = ov_thread_message_standard_create(OV_GENERIC_MESSAGE, 0);
    message->magic_bytes = ~OV_THREAD_MESSAGE_MAGIC_BYTES;
    testrun(message == ov_thread_message_free(message));
    message->magic_bytes = OV_THREAD_MESSAGE_MAGIC_BYTES;
    testrun(0 == ov_thread_message_free(message));

    /* Try to free any other message than OV_GENERIC_MESSAGE */
    message = ov_thread_message_standard_create(OV_GENERIC_MESSAGE + 1, 0);
    testrun(0 == ov_thread_message_free(message));

    /* Try to free with JSON attached */
    ov_json_value *json = ov_json_object();
    message = ov_thread_message_standard_create(OV_GENERIC_MESSAGE, json);
    testrun(0 == ov_thread_message_free(message));

    /* Try to free without free() callback */
    message = ov_thread_message_standard_create(OV_GENERIC_MESSAGE, 0);

    ov_thread_message *(*free_func)(ov_thread_message *) = message->free;

    message->free = 0;
    testrun(message == ov_thread_message_free(message));

    message->free = free_func;
    message = ov_thread_message_free(message);
    testrun(0 == message);

    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

OV_TEST_RUN("ov_thread_message_standard_create",
            test_ov_thread_message_standard_create,
            test_ov_thread_message_free);

/***
        ------------------------------------------------------------------------

        Copyright (c) 2021 German Aerospace Center DLR e.V. (GSOC)

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

        @author Michael J. Beer, DLR/GSOC
        @copyright (c) 2021 German Aerospace Center DLR e.V. (GSOC)

        ------------------------------------------------------------------------
*/
#ifndef OV_TEST_NET_H
#define OV_TEST_NET_H
/*----------------------------------------------------------------------------*/
#include "ov_json_value.h"

/*----------------------------------------------------------------------------*/

bool ov_test_send_msg_to(int fd, ov_json_value *jmsg);

/*----------------------------------------------------------------------------*/

/**
 * Checks whether 'msg' is a correct json message.
 * Checks whether its request name is 'event'.
 * Checks whether its an error response or not, depending on 'error_expected' .
 */
ssize_t ov_test_message_ok(char const *msg, size_t msglen, char const *event,
                           bool error_expected);

/*----------------------------------------------------------------------------*/

/**
 * Like ov_test_message_ok, but reads message from an fd.
 * Does not reassemble, thus not suited for long messages
 */
bool ov_test_expect(int fd, char const *event_name, bool error_expected);

/*----------------------------------------------------------------------------*/
#endif

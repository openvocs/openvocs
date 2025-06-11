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
        @file           ov_turn_channel_bind.h
        @author         Markus Töpfer

        @date           2022-01-21


        ------------------------------------------------------------------------
*/
#ifndef ov_turn_channel_bind_h
#define ov_turn_channel_bind_h

#define TURN_CHANNEL_BIND 0x009
#include "ov_stun_frame.h"
#include <stdbool.h>
#include <stdlib.h>

/*      ------------------------------------------------------------------------
 *
 *                              FRAME CHECKING
 *
 *      ------------------------------------------------------------------------
 */

/**
        Check if the frame method is bind.

        This function will only check the method contained in the frame and
        not validate the frame itself.

        @param frame           start of the frame buffer
        @param length          length of the attribute buffer (at least)
*/
bool ov_turn_method_is_channel_bind(const uint8_t *frame, size_t length);

#endif /* ov_turn_channel_bind_h */

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
        @file           ov_media_definitions.h
        @author         author
        @date           2021-06-10


        ------------------------------------------------------------------------
*/
#ifndef ov_media_definitions_h
#define ov_media_definitions_h

typedef enum {

  OV_MEDIA_ERROR = 0,
  OV_MEDIA_REQUEST,
  OV_MEDIA_OFFER,
  OV_MEDIA_ANSWER

} ov_media_type;

/*----------------------------------------------------------------------------*/

#define OV_MEDIA_KEY_REQUEST "request"
#define OV_MEDIA_KEY_OFFER "offer"
#define OV_MEDIA_KEY_ANSWER "answer"

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_media_type ov_media_type_from_string(const char *string);
const char *ov_media_type_to_string(ov_media_type type);

#endif /* ov_media_definitions_h */

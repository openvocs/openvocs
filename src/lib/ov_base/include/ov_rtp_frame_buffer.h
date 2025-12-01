/***

Copyright   2018        German Aerospace Center DLR e.V.,
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

     @author         Michael Beer

     The framebuffer keeps RTP frames.

     Whenever a frame is added, it is checked whether for the SSID of the
     frame there are already frames.
     If there are frames belonging to the particular SSID,
     they are stored ordered oldest first.
     The max number of frames stored is configurable.
     Other frames are silently dropped.

     The buffer can be asked to return a list of oldest frames, one frame
     per SSID.

 **/

#ifndef ov_rtp_frame_buffer_h
#define ov_rtp_frame_buffer_h

/*----------------------------------------------------------------------------*/

#include "ov_rtp_frame.h"
#include <stdbool.h>
#include <stddef.h>

#include "ov_list.h"

/******************************************************************************
 *                                  TYPEDEFS
 ******************************************************************************/

typedef struct {

  uint32_t magic_number;
  void *(*free)(void *);

} ov_rtp_frame_buffer;

typedef struct {

  size_t num_frames_to_buffer_per_stream;

} ov_rtp_frame_buffer_config;

/******************************************************************************
 *                                 FUNCTIONS
 ******************************************************************************/

ov_rtp_frame_buffer *
ov_rtp_frame_buffer_create(ov_rtp_frame_buffer_config config);

/*----------------------------------------------------------------------------*/

ov_rtp_frame_buffer *ov_rtp_frame_buffer_free(ov_rtp_frame_buffer *self);

/*----------------------------------------------------------------------------*/

/**
 * tries to add a frame to the frame buffer.
 * If another frame had to be replaced or this frame could not be inserted,
 * returns a pointer to the frame not (any more) in buffer. Otherwise 0.
 */
ov_rtp_frame *ov_rtp_frame_buffer_add(ov_rtp_frame_buffer *restrict self,
                                      ov_rtp_frame *frame);

/*----------------------------------------------------------------------------*/

ov_list *
ov_rtp_frame_buffer_get_current_frames(ov_rtp_frame_buffer *restrict self);

/*----------------------------------------------------------------------------*/

/**
 * Print frame buffer to a stream.
 */
void ov_rtp_frame_buffer_print(FILE *stream, ov_rtp_frame_buffer const *buffer);

/*----------------------------------------------------------------------------*/

#endif /* ov_rtp_frame_buffer_h */

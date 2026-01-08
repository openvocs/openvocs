/***

Copyright   2018,2020   German Aerospace Center DLR e.V.,
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

     \author             Michael J. Beer, DLR/GSOC <michael.beer@dlr.de>
     \date               2018-04-20

     Provides a framework to build processes that receive/generate PCM audio

 **/

#ifndef OV_GATEWAY_H
#define OV_GATEWAY_H

/*----------------------------------------------------------------------------*/

#include <ov_base/ov_buffer.h>
#include <ov_base/ov_json.h>

/******************************************************************************
 *                                  TYPEDEFS
 ******************************************************************************/

struct ov_gateway_struct;

typedef struct ov_gateway_struct ov_gateway;

/**
 * Basic configuration of all gateways.
 */
typedef struct {

    size_t frame_length_usecs;
    uint64_t sample_rate_hertz;

} ov_gateway_configuration;

/**
 * A gateway receives audio PCM signed 16bit (LE/BE depending on arch) and
 * processes it.
 * It further provides processed audio data.
 * The kind of audio data is dependent on the actual gateway implementation,
 */
struct ov_gateway_struct {

    /**
     * Gateway specific - depends on the actual gateway type
     */
    uint32_t magic_number;

    /**
     * Tear down and free the gateway.
     * This includes stopping all possibly running threads and the like.
     */
    ov_gateway *(*free)(ov_gateway *self);

    /**
     * Retrieve a chunk of pcm data.
     * The caller anounces the number of samples via requested_samples .
     * The gateway might provide for a different size, however,
     * by returing a buffer of different length (e.g. if not enough data
     * is available).
     *
     *  BEWARE: The byte order ir ARCH DEPENDENT!
     */
    ov_buffer *(*get_pcm_s16)(ov_gateway *self, size_t requested_samples);

    /**
     * Hand over audio data to the gateway.
     *
     * BEWARE: The byte order is ARCH DEPENDENT!
     *
     * @return false in case of error, true else
     */
    bool (*put_pcm_s16)(ov_gateway *self, const ov_buffer *data);

    ov_json_value *(*get_state)(ov_gateway *self);
};

#endif /* ov_gateway_h */

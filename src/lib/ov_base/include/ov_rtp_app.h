/***
        ------------------------------------------------------------------------

        Copyright (c) 2023 German Aerospace Center DLR e.V. (GSOC)

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
        @copyright (c) 2023 German Aerospace Center DLR e.V. (GSOC)

        ------------------------------------------------------------------------
*/
#ifndef OV_RTP_APP_H
#define OV_RTP_APP_H
/*----------------------------------------------------------------------------*/

#include "ov_constants.h"
#include "ov_event_loop.h"
#include "ov_rtp_frame.h"

/*----------------------------------------------------------------------------*/

struct ov_rtp_app;
typedef struct ov_rtp_app ov_rtp_app;

typedef struct {

    bool multicast;
    ov_socket_configuration rtp_socket;
    bool (*rtp_handler)(ov_rtp_frame *rtp_frame, void *userdata);
    void *rtp_handler_userdata;

} ov_rtp_app_config;

/*----------------------------------------------------------------------------*/

ov_rtp_app *ov_rtp_app_create(ov_event_loop *loop, ov_rtp_app_config cfg);
ov_rtp_app *ov_rtp_app_free(ov_rtp_app *self);

/*----------------------------------------------------------------------------*/

bool ov_rtp_app_get_rtp_socket(ov_rtp_app const *self,
                               ov_socket_configuration *scfg);

int ov_rtp_app_get_rtp_sd(ov_rtp_app const *self);

/*----------------------------------------------------------------------------*/

bool ov_rtp_app_send_to_sockaddr(int sd_to_send_from, ov_rtp_frame const *frame,
                                 struct sockaddr *dest, socklen_t dest_len);

typedef enum {
    IPV4 = AF_INET,
    IPV6 = AF_INET6,
} ov_ip_version;

bool ov_rtp_app_send_socket(int sd_to_send_from, ov_rtp_frame const *frame,
                            ov_socket_configuration target,
                            ov_ip_version ip_version);

/*----------------------------------------------------------------------------*/
#endif

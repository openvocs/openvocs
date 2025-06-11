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

        @author         Michael J. Beer, DLR/GSOC

        ------------------------------------------------------------------------
*/

#include "../include/ov_rtp_app.h"
#include "../include/ov_mc_socket.h"
#include "../include/ov_string.h"
#include <netdb.h>

/*----------------------------------------------------------------------------*/

#define MAGIC_BYTES 0x919A9900

/*----------------------------------------------------------------------------*/

#define DEFAULT_RTP_INTERFACE "0.0.0.0"

/*----------------------------------------------------------------------------*/

struct ov_rtp_app {

    uint32_t magic_bytes;

    ov_socket_configuration rtp_socket;
    bool multicast;

    ov_event_loop *loop;

    struct fd_handler_struct {

        int fd;
        bool (*handler)(ov_rtp_frame *rtp_frame, void *userdata);
        void *userdata;

    } rtp;
};

/*----------------------------------------------------------------------------*/

static ov_rtp_app const *as_rtp_app(void const *ptr) {

    ov_rtp_app const *rtpapp = ptr;

    if ((0 != ptr) && (MAGIC_BYTES == rtpapp->magic_bytes)) {
        return rtpapp;
    } else {
        return 0;
    }
}

/*----------------------------------------------------------------------------*/

static bool cb_rtp_io(int fd, uint8_t events, void *userdata) {

    UNUSED(events);

    struct fd_handler_struct *data = userdata;
    uint8_t buffer[OV_UDP_PAYLOAD_OCTETS];

    ov_log_debug("Received UDP data");

    ssize_t in = read(fd, buffer, sizeof(buffer));

    if (in < 0) {

        // read again
        return true;

    } else if (!ov_ptr_valid(data, "Cannot process incoming RTP data")) {

        return false;

    } else {

        ov_rtp_frame *frame = ov_rtp_frame_decode(buffer, (size_t)in);

        ov_log_debug("Received RTP frame");

        if (ov_ptr_valid(frame, "Could not decode RTP frame") &&
            (0 != data->handler)) {

            data->handler(frame, data->userdata);

        } else {

            frame = ov_rtp_frame_free(frame);
        }

        return true;
    }
}

/*----------------------------------------------------------------------------*/

static bool install_handler(ov_event_loop *loop,
                            struct fd_handler_struct *handler_struct,
                            bool (*low_level_handler)(int, uint8_t, void *)) {

    if (ov_ptr_valid(
            loop, "Cannot setup RT(C)P handler - invalid event loop") &&
        ov_ptr_valid(
            handler_struct, "Cannot setup RT(C)P handler - invalid handler") &&
        ov_cond_valid(-1 < handler_struct->fd,
                      "Cannot setup RT(C)P handler - invalid socket "
                      "handle")) {

        return ov_event_loop_set(
            loop,
            handler_struct->fd,
            OV_EVENT_IO_IN | OV_EVENT_IO_ERR | OV_EVENT_IO_CLOSE,
            handler_struct,
            low_level_handler);

    } else {
        return false;
    }
}

/*----------------------------------------------------------------------------*/

static int create_socket_for(ov_socket_configuration scfg,
                             bool mc,
                             ov_socket_error *err) {

    char const *type_str = "unicast";
    int sd = -1;

    if (mc) {

        type_str = "multicast";
        sd = ov_mc_socket(scfg);

    } else {

        sd = ov_socket_create(scfg, false, err);
    }

    if (sd > -1) {

        ov_log_info(
            "Openend %s socket at %s:%" PRIu16, type_str, scfg.host, scfg.port);
    } else {

        ov_log_error("Failed to open %s socket at %s:%" PRIu16,
                     type_str,
                     scfg.host,
                     scfg.port);
    }

    return sd;
}

/*----------------------------------------------------------------------------*/

static int close_socket(int fd, bool mc) {

    if (mc) {
        ov_mc_socket_drop_membership(fd);
    }
    close(fd);
    return -1;
}

/*----------------------------------------------------------------------------*/

static int open_rtcp_port(ov_socket_configuration const *rtp, bool mc) {

    if (!ov_ptr_valid(rtp, "Invalid RTP socket configuration: 0 pointer")) {
        return -1;
    } else {

        ov_socket_configuration rtcp = *rtp;
        ++rtcp.port;

        ov_socket_error err = {0};
        return create_socket_for(rtcp, mc, &err);
    }
}

/*----------------------------------------------------------------------------*/

static bool create_media_socket(ov_socket_configuration *media,
                                bool multicast,
                                int fds[2],
                                bool open_rtcp) {

    ov_socket_error err = {0};

    fds[0] = -1;
    fds[1] = -1;

    for (size_t num_tries = 0; 10 > num_tries; ++num_tries) {

        fds[0] = create_socket_for(*media, multicast, &err);

        if (0 != media->port % 2) {
            ov_log_info("RTP Port number %" PRIu16 " not even", media->port);
        }

        if (0 > fds[0]) {

            ov_log_error("Could not open media socket on %s:%" PRIu16 " :   %s",
                         ov_string_sanitize(media->host),
                         media->port,
                         strerror(err.err));
            return false;

        } else if (!ov_socket_get_config(fds[0], media, 0, &err)) {

            ov_log_error(
                "Could not get local media socket info: %s", strerror(err.err));
            close_socket(fds[0], multicast);
            return false;

        } else if (open_rtcp) {

            fds[1] = open_rtcp_port(media, multicast);

            if (-1 < fds[1]) {

                ov_log_info("Opened RTP/RTCP ports: %s:%" PRIu16 "/%" PRIu16,
                            media->host,
                            media->port,
                            media->port + 1);

                return true;

            } else {

                ov_log_error("Could not open RTCP socket on %s:%" PRIu16
                             " :   %s",
                             ov_string_sanitize(media->host),
                             media->port + 1,
                             strerror(err.err));

                close_socket(fds[0], multicast);
                fds[0] = -1;
            }

        } else {

            ov_log_info("Opened RTP port: %" PRIu16, media->port);
            return true;
        }
    }

    ov_log_error("Could not open RTP/RTCP ports");
    return false;
}

/*----------------------------------------------------------------------------*/

static void uninstall_handler(ov_event_loop *loop,
                              struct fd_handler_struct *handler_struct) {

    if (ov_ptr_valid(
            loop, "Cannot setup RT(C)P handler - invalid event loop") &&
        ov_ptr_valid(
            handler_struct, "Cannot setup RT(C)P handler - invalid handler")) {

        loop->callback.unset(loop, handler_struct->fd, 0);
    }
}

/*----------------------------------------------------------------------------*/

static bool set_handler_fd(struct fd_handler_struct *handler_struct, int fd) {

    if (ov_ptr_valid(
            handler_struct, "Cannot set FD on handler - invalid handler") &&
        ov_cond_valid(fd > -1, "Cannot set FD on handler - invalid FD")) {

        handler_struct->fd = fd;
        return true;

    } else {
        return false;
    }
}

/*----------------------------------------------------------------------------*/

static ov_socket_configuration or_default_rtp_socket(
    ov_socket_configuration cfg, bool set_default_port) {

    ov_socket_configuration res_cfg = cfg;

    if (0 == res_cfg.host[0]) {
        ov_log_warning(
            "Interface to listen on not set - using " DEFAULT_RTP_INTERFACE);
        ov_string_copy(res_cfg.host, DEFAULT_RTP_INTERFACE, OV_HOST_NAME_MAX);
    }

    if ((0 == res_cfg.port) && set_default_port) {
        ov_log_warning(
            "Port to listen on not set - using %i", OV_DEFAULT_RTP_PORT);
        res_cfg.port = OV_DEFAULT_RTP_PORT;
    }

    return res_cfg;
}

/*----------------------------------------------------------------------------*/

ov_rtp_app *ov_rtp_app_create(ov_event_loop *loop, ov_rtp_app_config cfg) {

    ov_rtp_app *self = calloc(1, sizeof(ov_rtp_app));
    self->magic_bytes = MAGIC_BYTES;

    self->rtp_socket = or_default_rtp_socket(cfg.rtp_socket, cfg.multicast);

    self->loop = loop;
    self->rtp.handler = cfg.rtp_handler;
    self->rtp.userdata = cfg.rtp_handler_userdata;
    self->multicast = cfg.multicast;

    int fds[2] = {-1, -1};

    if (ov_ptr_valid(cfg.rtp_handler, "No RTP handler") &&
        create_media_socket(&self->rtp_socket, cfg.multicast, fds, false) &&
        set_handler_fd(&self->rtp, fds[0]) &&
        install_handler(loop, &self->rtp, cb_rtp_io)) {

        return self;

    } else {

        return ov_rtp_app_free(self);
    }
}

/*----------------------------------------------------------------------------*/

ov_rtp_app *ov_rtp_app_free(ov_rtp_app *self) {

    if (0 != as_rtp_app(self)) {

        uninstall_handler(self->loop, &self->rtp);
        close_socket(self->rtp.fd, self->multicast);
        return ov_free(self);

    } else {
        return 0;
    }
}

/*----------------------------------------------------------------------------*/

bool ov_rtp_app_get_rtp_socket(ov_rtp_app const *self,
                               ov_socket_configuration *scfg) {

    if (ov_ptr_valid(scfg,
                     "Cannot get RTP socket - invalid target socket config "
                     "structure (0 pointer)") &&
        ov_ptr_valid(as_rtp_app(self),
                     "Cannot get RTP socket - invalid rtp app (0 pointer)")) {
        *scfg = self->rtp_socket;
        return true;
    } else {
        return false;
    }
}

/*----------------------------------------------------------------------------*/

int ov_rtp_app_get_rtp_sd(ov_rtp_app const *self) {

    if (ov_ptr_valid(as_rtp_app(self),
                     "Cannot get RTP socket - invalid rtp app (0 pointer)")) {

        return self->rtp.fd;

    } else {

        return -1;
    }
}

/*----------------------------------------------------------------------------*/

bool ov_rtp_app_send_to_sockaddr(int sd_to_send_from,
                                 ov_rtp_frame const *frame,
                                 struct sockaddr *dest,
                                 socklen_t dest_len) {

    if (ov_ptr_valid(frame, "Cannot send RTP frame - 0 pointer")) {

        ssize_t bytes = sendto(sd_to_send_from,
                               frame->bytes.data,
                               frame->bytes.length,
                               0,
                               (struct sockaddr *)&dest,
                               dest_len);

        if ((0 > bytes) || (frame->bytes.length != (size_t)bytes)) {

            ov_log_error("Could not send RTP frame: %s", strerror(errno));
            return false;

        } else {

            return true;
        }

    } else {

        return false;
    }
}

/*----------------------------------------------------------------------------*/

bool ov_rtp_app_send_socket(int sd_to_send_from,
                            ov_rtp_frame const *frame,
                            ov_socket_configuration target,
                            ov_ip_version ip_version) {

    socklen_t len = sizeof(struct sockaddr_in);
    sa_family_t address_family = AF_INET;

    switch (ip_version) {

        case IPV4:

            break;

        case IPV6:
            address_family = AF_INET6;
            len = sizeof(struct sockaddr_in6);
            break;
    };

    struct sockaddr_storage dest = {0};

    if (ov_cond_valid(ov_socket_fill_sockaddr_storage(
                          &dest, address_family, target.host, target.port),
                      "Faild to convert to sockaddr")) {

        return ov_cond_valid(
            ov_rtp_app_send_to_sockaddr(
                sd_to_send_from, frame, (struct sockaddr *)&dest, len),
            "Sending frame failed");

    } else {

        return false;
    }
}

/*----------------------------------------------------------------------------*/

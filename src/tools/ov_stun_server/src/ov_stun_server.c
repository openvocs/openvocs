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
        @file           ov_stun_server.c
        @author         Markus Töpfer

        @date           2021-10-25


        ------------------------------------------------------------------------
*/

#include <ov_base/ov_utils.h>
#include <stdlib.h>

#include <ov_base/ov_config.h>
#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_config_log.h>
#include <ov_base/ov_json.h>
#include <ov_base/ov_socket.h>

#include <ov_os/ov_os_event_loop.h>

#include <ov_stun/ov_stun_attribute.h>
#include <ov_stun/ov_stun_binding.h>
#include <ov_stun/ov_stun_frame.h>

#define IMPL_MAX_STUN_ATTRIBUTES 5

#define CONFIG_PATH                                                            \
    OPENVOCS_ROOT                                                              \
    "/src/tools/ov_stun_server/config/default_config.json"

static bool debug = false;

/*----------------------------------------------------------------------------*/

static bool process_stun(int socket, uint8_t events, void *userdata) {

    ov_event_loop *loop = ov_event_loop_cast(userdata);
    ssize_t send = -1;
    const char *error = NULL;

    size_t size = OV_UDP_PAYLOAD_OCTETS;
    ov_socket_data data = {0};

    uint8_t buffer[size];
    memset(buffer, 0, size);

    uint8_t *attr[IMPL_MAX_STUN_ATTRIBUTES] = {0};

    if (!loop || (socket < 0)) goto error;

    if (!(events & OV_EVENT_IO_IN)) goto close;

    struct sockaddr_storage sa = {0};
    socklen_t sa_len = sizeof(sa);

    if (debug) ov_socket_get_data(socket, &data, NULL);

    ssize_t in =
        recvfrom(socket, buffer, size, 0, (struct sockaddr *)&sa, &sa_len);

    if (in < 0) goto done;

    // ignore any non stun io
    if (!ov_stun_frame_is_valid(buffer, in)) {

        error = "invalid STUN frame";
        goto done;
    }

    // ignore any non binding
    if (!ov_stun_method_is_binding(buffer, in)) {

        error = "method not binding";
        goto done;
    }

    // only requests supported
    if (!ov_stun_frame_class_is_request(buffer, in)) {

        error = "not a request";
        goto done;
    }

    // check we received no more attributes as supported
    if (!ov_stun_frame_slice(buffer, in, attr, IMPL_MAX_STUN_ATTRIBUTES)) {

        error = "attributes failed";
        goto done;
    }

    uint16_t type = 0;

    // we only support a fingerprint as attribute
    for (size_t i = 0; i < IMPL_MAX_STUN_ATTRIBUTES; i++) {

        if (attr[i] == NULL) break;

        type = ov_stun_attribute_get_type(attr[i], 4);

        switch (type) {

            case STUN_SOFTWARE:
            case STUN_FINGERPRINT:
                break;

            default:

                error = "unsupported attributes";
                goto done;
        }
    }

    // fingerprint is optinal
    if (!ov_stun_check_fingerprint(
            buffer, in, attr, IMPL_MAX_STUN_ATTRIBUTES, false)) {

        error = "fingerprint failed";
        goto done;
    }

    /*
     *  We do use the read buffer to create the response.
     *  This will limit memory operations to the minimum amount.
     *
     *  This means, we need to change the class to response,
     *  and keep the magic_cookie, method and transaction id.
     *
     *  We will copy the xor mapped address as first attribute after
     *  the header, and reset the length attribute within the header.
     *
     *  This is a zero copy implementaion within the read buffer.
     */

    if (!ov_stun_frame_set_success_response(buffer, in)) {

        error = "set success failed";
        goto done;
    }

    if (!ov_stun_xor_mapped_address_encode(
            buffer + 20, size - 20, buffer, NULL, &sa)) {

        error = "set XOR mapped address failed";
        goto done;
    }

    size_t out = 20 + ov_stun_xor_mapped_address_encoding_length(&sa);
    OV_ASSERT(out < size);

    if (!ov_stun_frame_set_length(buffer, size, out - 20)) {

        error = "set length failed";
        goto done;
    }

    // add a fingerprint
    if (!ov_stun_add_fingerprint(buffer, size, buffer + out, NULL)) {

        error = "set fingerprint failed";
        goto done;
    }

    out += ov_stun_fingerprint_encoding_length();

    // just to be sure, we nullify the rest of the buffer
    memset(buffer + out, 0, size - out);

    send = sendto(socket,
                  buffer,
                  out,
                  0,
                  (const struct sockaddr *)&sa,
                  sizeof(struct sockaddr_storage));

    /*
     *  We do ignore if we actually send the reponse or not,
     *  as the STUN protocol is not reliable anyway.
     */

done:

    data = ov_socket_data_from_sockaddr_storage(&sa);

    if (-1 != send) {

        ov_log_debug("STUN send response to %s:%i", data.host, data.port);

    } else if (debug) {

        ov_log_debug("STUN send failed to %s:%i - error %s",
                     data.host,
                     data.port,
                     error);
    }

    return true;

close:

    loop->callback.unset(loop, socket, NULL);
    close(socket);
    ov_log_error("Closed socket %i\n", socket);

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static int open_socket(ov_event_loop *loop, ov_socket_configuration config) {

    int socket = ov_socket_create(config, false, NULL);

    if (-1 == socket) {
        goto done;
    }

    if (!ov_socket_ensure_nonblocking(socket)) {
        close(socket);
        socket = -1;
        goto done;
    }

    if (!loop->callback.set(loop,
                            socket,
                            OV_EVENT_IO_IN | OV_EVENT_IO_CLOSE,
                            loop,
                            process_stun)) {

        close(socket);
        socket = -1;
    }

done:

    if (debug) {

        if (-1 == socket) {

            ov_log_error(
                "STUN error open socket %s:%i\n", config.host, config.port);

        } else {

            ov_log_info("STUN open socket %s:%i\n", config.host, config.port);
        }
    }

    return socket;
}

/*----------------------------------------------------------------------------*/

static bool open_array_socket(void *item, void *data) {

    ov_json_value *val = ov_json_value_cast(item);
    ov_event_loop *loop = ov_event_loop_cast(data);
    if (!item || !loop) return false;

    ov_socket_configuration config = ov_socket_configuration_from_json(
        val, (ov_socket_configuration){.type = UDP});

    open_socket(loop, config);
    return true;
}

/*----------------------------------------------------------------------------*/

static bool start_ports(ov_event_loop *loop, ov_json_value *jconfig) {

    if (!loop || !jconfig) goto error;

    const ov_json_value *stun = ov_json_object_get(jconfig, OV_KEY_STUN);

    ov_socket_configuration config = ov_socket_configuration_from_json(
        ov_json_object_get(stun, OV_KEY_SOCKET),
        (ov_socket_configuration){
            .host = "localhost", .type = UDP, .port = 3478});

    if (ov_json_get(stun, "/range")) {

        uint64_t start = ov_json_number_get(ov_json_get(stun, "/range/start"));
        uint64_t end = ov_json_number_get(ov_json_get(stun, "/range/end"));

        for (uint64_t i = start; i <= end; i++) {

            config.port = i;
            open_socket(loop, config);
        }

    } else if (-1 == open_socket(loop, config)) {

        goto error;
    }

    ov_json_value const *array = ov_json_get(stun, "/" OV_KEY_SOCKETS);

    if (array)
        ov_json_array_for_each((ov_json_value *)array, loop, open_array_socket);

    return true;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

int main(int argc, char **argv) {

    ov_json_value *config = NULL;

    ov_event_loop *loop = NULL;

    const char *path = ov_config_path_from_command_line(argc, argv);

    if (!path) path = CONFIG_PATH;

    config = ov_config_load(path);
    if (!config) goto error;

    if (!ov_config_log_from_json(config)) goto error;

    if (ov_json_is_true(ov_json_get(config, "/stun/debug"))) debug = true;

    uint32_t max_sockets =
        ov_socket_get_max_supported_runtime_sockets(UINT16_MAX);

    /* Initiate the event loop to be used */

    loop = ov_os_event_loop(
        (ov_event_loop_config){.max.sockets = max_sockets, .max.timers = 10});

    if (!loop) goto error;

    if (!start_ports(loop, config)) goto error;

    /*  Run event loop */
    loop->run(loop, OV_RUN_MAX);

    ov_json_value_free(config);
    ov_event_loop_free(loop);
    return EXIT_SUCCESS;

error:
    ov_json_value_free(config);
    ov_event_loop_free(loop);
    return EXIT_FAILURE;
}

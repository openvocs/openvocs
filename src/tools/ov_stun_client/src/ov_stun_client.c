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
        @file           ov_stun_client.c
        @author         Markus Töpfer

        @date           2021-10-25


        ------------------------------------------------------------------------
*/

#include <ov_base/ov_utils.h>
#include <stdlib.h>

#include <ov_base/ov_config.h>
#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_config_log.h>
#include <ov_base/ov_dict.h>
#include <ov_base/ov_json.h>
#include <ov_base/ov_socket.h>

#include <ov_os/ov_os_event_loop.h>

#include <ov_stun/ov_stun_attribute.h>
#include <ov_stun/ov_stun_binding.h>
#include <ov_stun/ov_stun_frame.h>

#define IMPL_MAX_STUN_ATTRIBUTES 5
#define IMPL_RUNTIME_USECS 1 * 1000 * 1000

#define CONFIG_PATH                                                            \
    OPENVOCS_ROOT                                                              \
    "/src/tools/ov_stun_client/config/default_config.json"

/*----------------------------------------------------------------------------*/

static ov_dict *global_transactions;
static bool stop_on_finished = false;

/*----------------------------------------------------------------------------*/

static bool process_stun(int socket, uint8_t events, void *userdata) {

    ov_event_loop *loop = ov_event_loop_cast(userdata);

    size_t size = OV_UDP_PAYLOAD_OCTETS;

    uint8_t buffer[size];
    memset(buffer, 0, size);

    uint8_t *attr[IMPL_MAX_STUN_ATTRIBUTES] = {0};

    if (!loop || (socket < 0))
        goto error;

    if (!(events & OV_EVENT_IO_IN))
        goto close;

    ov_socket_data remote = {0};
    struct sockaddr_storage sa = {0};
    socklen_t sa_len = sizeof(sa);

    ssize_t in =
        recvfrom(socket, buffer, size, 0, (struct sockaddr *)&sa, &sa_len);

    if (in < 0)
        goto done;

    remote = ov_socket_data_from_sockaddr_storage(&sa);

    // ignore any non stun io
    if (!ov_stun_frame_is_valid(buffer, in)) {
        goto done;
    }

    // ignore any non binding
    if (!ov_stun_method_is_binding(buffer, in)) {
        goto done;
    }

    uint8_t transaction_id[13] = {0};
    memcpy(transaction_id, ov_stun_frame_get_transaction_id(buffer, in), 12);
    ov_dict_del(global_transactions, transaction_id);

    if (!ov_stun_frame_class_is_success_response(buffer, in)) {
        goto done;
    }

    // check we received no more attributes as supported
    if (!ov_stun_frame_slice(buffer, in, attr, IMPL_MAX_STUN_ATTRIBUTES)) {
        goto done;
    }

    uint16_t type = 0;

    for (size_t i = 0; i < IMPL_MAX_STUN_ATTRIBUTES; i++) {

        if (attr[i] == NULL)
            break;

        type = ov_stun_attribute_get_type(attr[i], 4);

        switch (type) {

        case STUN_SOFTWARE:
        case STUN_FINGERPRINT:
        case STUN_XOR_MAPPED_ADDRESS:
            break;

        default:
            goto done;
        }
    }

    if (!ov_stun_check_fingerprint(buffer, in, attr, IMPL_MAX_STUN_ATTRIBUTES,
                                   false)) {
        goto done;
    }

    uint8_t *xmap = ov_stun_attributes_get_type(attr, IMPL_MAX_STUN_ATTRIBUTES,
                                                STUN_XOR_MAPPED_ADDRESS);

    if (!xmap) {
        goto done;
    }

    ov_socket_data xor_mapped = {0};
    struct sockaddr_storage *xor_ptr = &xor_mapped.sa;

    if (!ov_stun_xor_mapped_address_decode(xmap, in - (xmap - buffer), buffer,
                                           &xor_ptr)) {
        goto done;
    }

    xor_mapped = ov_socket_data_from_sockaddr_storage(&xor_mapped.sa);

    fprintf(stdout, "STUN success %s:%i from %s:%i\n", xor_mapped.host,
            xor_mapped.port, remote.host, remote.port);

    if (ov_dict_is_empty(global_transactions))
        goto close;

done:
    return true;

close:

    loop->callback.unset(loop, socket, NULL);
    close(socket);
    fprintf(stdout, "DONE - closed socket %i\n", socket);
    loop->stop(loop);
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool check_server(ov_event_loop *loop, int client,
                         struct sockaddr_storage *local,
                         ov_socket_configuration config) {

    uint8_t *transaction_id = NULL;

    size_t size = OV_UDP_PAYLOAD_OCTETS;

    uint8_t buffer[size];
    memset(buffer, 0, size);
    uint8_t *next = NULL;

    if (!loop || (client < 1))
        goto error;

    struct sockaddr_storage sa;
    socklen_t socklen = sizeof(sa);

    if (!ov_socket_configuration_to_sockaddr(config, &sa, &socklen))
        goto error;

    transaction_id = calloc(13, sizeof(uint8_t));

    if (!ov_stun_frame_generate_transaction_id(transaction_id))
        goto error;

    intptr_t value = config.port;

    if (!ov_dict_set(global_transactions, transaction_id, (void *)value, NULL))
        goto error;

    if (!ov_stun_generate_binding_request_plain(
            buffer, size, &next, transaction_id, NULL, 0, true)) {

        goto failed;
    }

    socklen = sizeof(struct sockaddr_in);
    if (local->ss_family == AF_INET6)
        socklen = sizeof(struct sockaddr_in6);

    ssize_t bytes = sendto(client, buffer, next - buffer, 0,
                           (struct sockaddr *)&sa, socklen);

    if (bytes > 0) {

        // fprintf(stdout, "SEND request to %s:%i\n", config.host, config.port);

    } else {

        goto failed;
    }

    return true;

failed:

    fprintf(stdout, "SEND failed to %s:%i error %i|%s\n", config.host,
            config.port, errno, strerror(errno));
    ov_dict_del(global_transactions, transaction_id);

error:
    return false;
}

/*----------------------------------------------------------------------------*/

struct container {

    int socket;
    struct sockaddr_storage sa;
    ov_event_loop *loop;
};

/*----------------------------------------------------------------------------*/

static bool array_check_server(void *item, void *data) {

    ov_json_value *val = ov_json_value_cast(item);
    struct container *c = (struct container *)data;
    if (!val || !c)
        return false;

    ov_socket_configuration server = ov_socket_configuration_from_json(
        val, (ov_socket_configuration){.type = UDP});

    check_server(c->loop, c->socket, &c->sa, server);
    c->loop->run(c->loop, OV_RUN_ONCE);
    return true;
}

/*----------------------------------------------------------------------------*/

static bool check_ports(ov_event_loop *loop, ov_json_value *jconfig) {

    if (!loop || !jconfig)
        goto error;

    const ov_json_value *stun = ov_json_object_get(jconfig, OV_KEY_STUN);

    ov_socket_configuration client = ov_socket_configuration_from_json(
        ov_json_object_get(stun, OV_KEY_CLIENT),
        (ov_socket_configuration){.host = "0.0.0.0", .type = UDP, .port = 0});

    int socket = ov_socket_create(client, false, NULL);
    struct sockaddr_storage sa = {0};

    if ((-1 == socket) ||
        !ov_socket_get_sockaddr_storage(socket, &sa, NULL, NULL))
        goto error;

    if (!loop->callback.set(loop, socket, OV_EVENT_IO_IN | OV_EVENT_IO_CLOSE,
                            loop, process_stun)) {

        fprintf(stdout, "open socket %i - event cb failed\n", socket);
        close(socket);
        goto error;
    }

    fprintf(stdout, "opened client socket %i\n", socket);

    ov_json_value const *value =
        ov_json_get(stun, "/" OV_KEY_SERVER "/" OV_KEY_SOCKET);

    if (value) {

        ov_socket_configuration server = ov_socket_configuration_from_json(
            value, (ov_socket_configuration){
                       .host = "localhost", .type = UDP, .port = 3478});

        if (ov_json_get(stun, "/server/range")) {

            uint64_t start =
                ov_json_number_get(ov_json_get(stun, "/server/range/start"));
            uint64_t end =
                ov_json_number_get(ov_json_get(stun, "/server/range/end"));

            for (uint64_t i = start; i <= end; i++) {

                server.port = i;
                check_server(loop, socket, &sa, server);
                loop->run(loop, OV_RUN_ONCE);
            }

        } else if (!check_server(loop, socket, &sa, server)) {
            goto error;
        }
    }

    value = ov_json_get(stun, "/" OV_KEY_SERVERS);

    if (value) {

        struct container c =
            (struct container){.socket = socket, .sa = sa, .loop = loop};

        ov_json_array_for_each((ov_json_value *)value, &c, array_check_server);
    }

    stop_on_finished = true;
    return true;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

static void *log_failed_port(void *data) {

    if (NULL == data)
        return NULL;

    intptr_t port = (intptr_t)data;
    fprintf(stdout, "PORT w/o response: %" PRIiPTR "\n", port);
    return NULL;
}

/*----------------------------------------------------------------------------*/

int main(int argc, char **argv) {

    ov_json_value *config = NULL;

    ov_event_loop *loop = NULL;
    global_transactions = ov_dict_create(ov_dict_string_key_config(255));
    if (!global_transactions)
        goto error;

    const char *path = ov_config_path_from_command_line(argc, argv);

    if (!path)
        path = CONFIG_PATH;

    config = ov_config_load(path);
    if (!config)
        goto error;

    if (!ov_config_log_from_json(config))
        goto error;

    uint32_t max_sockets = 10;
    uint64_t max_runtime =
        ov_json_number_get(ov_json_get(config, "/stun/max_wait_usecs"));
    if (0 == max_runtime)
        max_runtime = IMPL_RUNTIME_USECS;

    /* Initiate the event loop to be used */

    loop = ov_os_event_loop((ov_event_loop_config){.max.sockets = max_sockets,
                                                   .max.timers = max_sockets});

    if (!loop)
        goto error;

    if (!check_ports(loop, config))
        goto error;

    /*  Run event loop */

    if (stop_on_finished && ov_dict_is_empty(global_transactions)) {

        // already done

    } else {

        // wait for max_runtime for reponses
        loop->run(loop, max_runtime);
    }

    global_transactions->config.value.data_function.free = log_failed_port;
    int64_t missed = ov_dict_count(global_transactions);

    ov_json_value_free(config);
    ov_event_loop_free(loop);
    ov_dict_free(global_transactions);
    fprintf(stdout, "MISSED COUNT %" PRIi64 "\n", missed);
    return EXIT_SUCCESS;

error:
    ov_json_value_free(config);
    ov_event_loop_free(loop);
    ov_dict_free(global_transactions);
    return EXIT_FAILURE;
}

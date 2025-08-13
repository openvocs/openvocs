/***
        ------------------------------------------------------------------------

        Copyright (c) 2025 German Aerospace Center DLR e.V. (GSOC)

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
        @author         Michael J. Beer

        ------------------------------------------------------------------------
*/
#include <stdio.h>
#include <ov_base/ov_utils.h>
#include <stdarg.h>
#include <ov_base/ov_socket.h>
#include <ov_base/ov_buffer.h>

/*----------------------------------------------------------------------------*/

FILE *outstream = 0;

/*----------------------------------------------------------------------------*/

static void print(char const *msg, ...) {

    va_list args;
    va_start(args, msg);

    fprintf(outstream, "%lld: ", (long long)time(0));
    vfprintf(outstream, msg, args);

    va_end(args);

}

/*----------------------------------------------------------------------------*/

static ov_buffer *get_next_message(FILE *input) {

    UNUSED(input);

    size_t period = 10; // MUST be bigger than 8...
    size_t len = 100 * 1000 * 100 * period;
    char *str = calloc(1, len);

    for(size_t i = 0; i < len; i += period) {

        str[i] = 'a';
        str[i + 1] = 'b';
        str[i + 2] = 'a';
        str[i + 5] = '1';
        str[i + 6] = '9';
        str[i + 7] = '1';

    }

    ov_buffer *buf = ov_buffer_create(len);
    buf->start = (uint8_t *)str;
    buf->length = len;

    return buf;

}

/*----------------------------------------------------------------------------*/

int main(int argc, char *argv[]) {

    outstream = stderr;

    char const *input_file  = "STDIN";
    FILE *input = stdin;

    uint32_t delay_secs = 5;

    ov_socket_configuration server = {
        .host = "127.0.0.1",
        .port = 1111,
        .type = TCP,
    };

    if((argc > 1) && (0 != strcmp(argv[1], "-"))) {
        input_file = argv[1];
        input = fopen(input_file, "r");
    }

    if(0 == input) {
        print("Could not open %s\n", input_file);
        exit(EXIT_FAILURE);
    }

    ov_socket_error serr = {0};
    int send_fd = ov_socket_create(server, true, &serr);

    if (0 > send_fd) {
        print("Could not connect to %s:%" PRIu16 "  - %s\n", server.host,
              server.port, strerror(serr.err));
        exit(EXIT_FAILURE);
    }

    if (argc > 3) {
        strncpy(server.host, argv[2], sizeof(server.host));
        server.port = atoi(argv[3]);
    }

    print("Reading from %s\n", input_file);
    print("Sending to %s:%" PRIu16 "\n", server.host, server.port);

    ov_buffer *message = get_next_message(input);

    while (0 != message) {
        usleep(delay_secs * 1000 * 1000);

        ssize_t written = 0;
        size_t to_write_len = message->length;
        void *to_write = message->start;

        while (to_write_len > 0) {
            written = write(send_fd, to_write, to_write_len);

            if (written > -1) {
                to_write = to_write + (size_t)written;
                to_write_len -= (size_t)written;
                print("Wrote %zd bytes - %zu to go...\n", written,
                      to_write_len);
            } else {
                print("Could not write: %s\n", strerror(errno));
                break;
            }
        }

        message = ov_buffer_free(message);
        message = get_next_message(input);
    }

    fclose(input);
    input = 0;

    return EXIT_SUCCESS;
}

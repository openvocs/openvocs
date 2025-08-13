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
#include <ov_base/ov_buffer.h>
#include <ov_base/ov_socket.h>
#include <ov_base/ov_utils.h>
#include <stdarg.h>
#include <stdio.h>

/*----------------------------------------------------------------------------*/

FILE *outstream = 0;
FILE *err_outstream = 0;

/*----------------------------------------------------------------------------*/

typedef enum { SND, RCV, INVALID } State;

State state_to_replay = RCV;

/*----------------------------------------------------------------------------*/

static int print_timestamp_nocheck(FILE *msg_out) {

    struct timespec ts = {0};

    if (!timespec_get(&ts, TIME_UTC)) {
        return -1;
    }

    struct tm broken_down_time = {0};

    if (0 == gmtime_r(&ts.tv_sec, &broken_down_time)) {
        return -1;
    }

    char time_str[25] = {0};

    if (!strftime(time_str, sizeof(time_str), "%FT%T", &broken_down_time)) {
        return -1;
    }

    return fprintf(msg_out, "%s.%.6ldZ", time_str, ts.tv_nsec);
}

/*----------------------------------------------------------------------------*/

static void print_to(FILE *out, char const *msg, ...) {

    va_list args;
    va_start(args, msg);

    print_timestamp_nocheck(out);
    vfprintf(out, msg, args);

    va_end(args);

}

#define print(...) print_to(outstream, __VA_ARGS__)
#define printerr(...) print_to(err_outstream, __VA_ARGS__)

//static void print(char const *msg, ...) {
//    va_list args;
//    va_start(args, msg);
//    print_to(outstream, msg, args);
//    va_end(args);
//}
//
//static void printerr(char const *msg, ...) {
//    va_list args;
//    va_start(args, msg);
//    print_to(err_outstream, msg, args);
//    va_end(args);
//}

/*----------------------------------------------------------------------------*/

static char const *state_to_str(State state) {
    switch (state) {
        case SND:
            return "SND";
        case RCV:
            return "RCV";
        default:
            return "INVALID";
    }
}

/*----------------------------------------------------------------------------*/

static unsigned char get_char(FILE *input) {
    int i = fgetc(input);

    if ((i < 0) || (i == EOF)) {
        printerr("During reading: Error or EOF occured\n");
        exit(EXIT_SUCCESS);
    } else {
        return (unsigned char)i;
    }
}

/*----------------------------------------------------------------------------*/

static unsigned char line[10000] = {0};

// Always points to end of string insofar as line[line_len] = 0 is the
// terminal zero == first char following message
static size_t line_len = 0;

static size_t max_line_len = sizeof(line) / sizeof(line[0]);

#define CR '\r'
#define LF '\n'

static size_t read_line(FILE *input) {
    // in here, always points to the NEXT slot to write to
    size_t i = 0;

    char c = get_char(input);
    while (c != 0) {
        line[i++] = c;
        if (c == LF) {
            break;
        } else {
            c = get_char(input);
        }

        if (i >= max_line_len) {
            printerr(
                "Line way too long %zu chars - surpasing maximum chars of %zu",
                i, max_line_len);
            abort();
        }
    };

    line[i] = 0;
    line_len = i;

    printerr("Got line '%s'\n", line);

    return i;
}

/*----------------------------------------------------------------------------*/

static State line_to_state() {
    bool digits_found = false;

    size_t i = 0;
    while (i < line_len) {
        if (0 != strchr("0123456789", line[++i])) {
            digits_found = true;
        } else {
            break;
        }
    }

    if (!digits_found) {
        printerr("No digits found in %s\n", line);
        return INVALID;
    }

    printerr("Digits in %s\n", line);
    // Now should come a string like <XXX>\n

    // So we require at least 6 more chars, and next char should be a '<'
    if (((i + 5) > line_len) || (line[i] != '<')) {
        printerr(
            "After digits: Line too short or '<' missing: %s  - %c should be "
            "'<'\n",
            line, line[i]);
        return INVALID;
    }

    printerr("After digits: Line long enough and '<' found: %s\n", line);

    if (line[i + 1] == 'S') {
        // So the line is either invalid or SND
        if ((line[i + 2] == 'N') && (line[i + 3] == 'D') &&
            (line[i + 4] == '>')) {
            return SND;
        } else {
            return INVALID;
        }

    } else if (line[i + 1] == 'R') {
        // So the line is either invalid or RCV
        if ((line[i + 2] == 'C') && (line[i + 3] == 'V') &&
            (line[i + 4] == '>')) {
            return RCV;
        } else {
            return INVALID;
        }

    } else {
        // Other garbage
        return INVALID;
    }
}

/*----------------------------------------------------------------------------*/

static State skip_header(FILE *input) {
    State state = INVALID;

    while (true) {
        size_t len = read_line(input);

        // Check if we can skip the line - We can skip anything that does not
        // end in CR LF

        if ((len > 1) && (CR == line[len - 2]) && (LF == line[len - 1])) {
            printerr("Detected start of %s message\n", state_to_str(state));
            return state;
        } else {
            state = line_to_state();
        }

        printerr("Skipped %s - state is %s\n", line, state_to_str(state));
        line[0] = 0;
        line_len = 0;
    }

    return state;
}

/*----------------------------------------------------------------------------*/

static ov_buffer *get_next_message(FILE *input, char *first_line) {
    State state = INVALID;

    while (state != state_to_replay) {
        state = skip_header(input);

        printerr("get_next_message: Got a %s message\n", state_to_str(state));
    }

    // Now the next line is already in the line - buffer
    printerr("get_next_message: Creating message to send...\n\n\n");

    size_t total_len = 0;
    char message[1000 * 1000] = {
        0};  // That's one MB, should suffice for a SIP message

    char *write_ptr = message;

    if (0 != first_line) {
        memcpy(first_line, line, line_len);
    }

    while ((line_len > 1) && (CR == line[line_len - 2]) &&
           (LF == line[line_len - 1])) {
        memcpy(write_ptr, line, line_len);
        write_ptr += line_len;
        total_len += line_len;

        line[0] = 0;
        line_len = 0;

        read_line(input);
    }

    ov_buffer *buf = ov_buffer_create(total_len);
    memcpy(buf->start, message, total_len);
    buf->length = total_len;

    return buf;
}

/*----------------------------------------------------------------------------*/

int main(int argc, char *argv[]) {
    outstream = stdout;
    err_outstream = stderr;

    char const *input_file = "STDIN";
    FILE *input = stdin;

    uint32_t delay_secs = 5;

    ov_socket_configuration server = {
        .host = "127.0.0.1",
        .port = 1111,
        .type = TCP,
    };

    if ((argc > 1) && (0 != strcmp(argv[1], "-"))) {
        input_file = argv[1];
        input = fopen(input_file, "r");
    }

    if (0 == input) {
        printerr("Could not open %s\n", input_file);
        exit(EXIT_FAILURE);
    }

    ov_socket_error serr = {0};
    int send_fd = ov_socket_create(server, true, &serr);

    if (0 > send_fd) {
        printerr("Could not connect to %s:%" PRIu16 "  - %s\n", server.host,
              server.port, strerror(serr.err));
        exit(EXIT_FAILURE);
    }

    if (argc > 3) {
        strncpy(server.host, argv[2], sizeof(server.host));
        server.port = atoi(argv[3]);
    }

    print("Reading from %s\n", input_file);
    print("Sending to %s:%" PRIu16 "\n", server.host, server.port);

    char first_line[10000] = {0};
    size_t first_line_capacity = sizeof(first_line) / sizeof(first_line[0]);

    ov_buffer *message = get_next_message(input, first_line);

    while (0 != message) {
        usleep(delay_secs * 1000 * 1000);

        print("Sending %s\n", first_line);
        memset(first_line, 0, first_line_capacity);

        ssize_t written = 0;
        size_t to_write_len = message->length;
        void *to_write = message->start;

        while (to_write_len > 0) {
            written = write(send_fd, to_write, to_write_len);

            if (written > -1) {
                to_write = to_write + (size_t)written;
                to_write_len -= (size_t)written;
                printerr("Wrote %zd bytes - %zu to go...\n", written,
                      to_write_len);
            } else {
                printerr("Could not write: %s\n", strerror(errno));
                break;
            }
        }

        message = ov_buffer_free(message);
        message = get_next_message(input, first_line);
    }

    fclose(input);
    input = 0;

    return EXIT_SUCCESS;
}

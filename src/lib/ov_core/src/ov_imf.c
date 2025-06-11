/***
        ------------------------------------------------------------------------

        Copyright (c) 2019 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_imf.c
        @author         Markus Toepfer

        @date           2019-07-19

        @ingroup        ov_core

        @brief          Implementation of


        ------------------------------------------------------------------------
*/
#include "ov_imf.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/time.h>
#include <time.h>

const char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

const char *month[] = {"Jan",
                       "Feb",
                       "Mar",
                       "Apr",
                       "May",
                       "Jun",
                       "Jul",
                       "Aug",
                       "Sep",
                       "Oct",
                       "Nov",
                       "Dec"};

/*----------------------------------------------------------------------------*/

static int get_day_index(char *string) {

    if (!string) goto error;

    for (size_t i = 0; i < 7; i++) {

        if (0 == strncasecmp(string, days[i], 3)) return i;
    }

error:
    return -1;
}

/*----------------------------------------------------------------------------*/

static int get_month_index(char *string) {

    if (!string) goto error;

    for (size_t i = 0; i < 12; i++) {

        if (0 == strncasecmp(string, month[i], 3)) return i;
    }

error:
    return -1;
}

/*----------------------------------------------------------------------------*/

bool ov_imf_tm_to_timestamp(const struct tm *source,
                            char *buffer,
                            size_t size,
                            char **next,
                            const char *zone) {

    if (!source || !buffer || size < 30) goto error;

    int r = snprintf(buffer,
                     size,
                     "%s, %02d %s %04i %02d:%02d:%02d %s",
                     days[source->tm_wday],
                     source->tm_mday,
                     month[source->tm_mon],
                     1900 + source->tm_year,
                     source->tm_hour,
                     source->tm_min,
                     source->tm_sec,
                     zone ? zone : "GMT");

    if (r < 1) goto error;

    if (next) *next = buffer + r;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_imf_write_timestamp(char *start, size_t size, char **next) {

    if (!start || size < 30) goto error;

    // Sun, 06 Nov 1994 08:49:37 GMT    ; IMF-fixdate
    // Fri, 19 Jul 2019 14:46:52 GMT
    struct timeval tv;

    if (0 != gettimeofday(&tv, NULL)) return NULL;

    struct tm *current = gmtime(&tv.tv_sec);

    return ov_imf_tm_to_timestamp(current, start, size, next, NULL);
error:
    return false;
}

/*----------------------------------------------------------------------------*/

char *move_folded_white_space(char *start, size_t max) {

    if (!start || max == 0) goto error;

    char *ptr = start;
    size_t len = 0;

    while (true) {

        if (!isspace(*ptr)) return ptr;

        ptr++;

        if (ptr[0] == '\r') {

            len = ptr - start;

            if (len < 3 || (len + 3) >= max) break;

            if (ptr[1] != '\n') break;

            if (ptr[2] == ' ') break;

            ptr += 3;
        }
    }

error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

struct tm ov_imf_timestamp_to_tm(const char *start, size_t size, char **next) {

    if (!start || size < 30) goto error;

    /*
     *      Problem is locale setting for Day and Month,
     *      we want to avoid any locale issue and use
     *      the English names only.
     *
     *      Easy solution programming wise would be to change
     *      the locale of the host we are running at.
     *      But that may cause unrelated (non thread safe) problems.
     *
     *      So we will copy required time information to gather
     *      the time using a system function.
     */

    struct tm parsed = {0};
    memset(&parsed, 0, sizeof(struct tm));

    char *ptr = (char *)start;
    char *end = NULL;

    // Sun, 06 Nov 1994 08:49:37 GMT

    parsed.tm_wday = get_day_index(ptr);
    if (parsed.tm_wday < 0) goto error;

    end = memchr(ptr, ',', size - (size_t)(ptr - start));
    if (!end) goto error;
    ptr = end + 1;
    ptr = move_folded_white_space(ptr, size - (size_t)(ptr - start));
    if (!ptr) goto error;

    // DAY
    parsed.tm_mday = strtol(ptr, &end, 10);
    if (*end != ' ') goto error;
    if ((parsed.tm_mday < 0) || (parsed.tm_mday > 31)) goto error;
    ptr = end + 1;
    ptr = move_folded_white_space(ptr, size - (size_t)(ptr - start));
    if (!ptr) goto error;

    // MONTH
    parsed.tm_mon = get_month_index(ptr);
    if (parsed.tm_mon < 0) goto error;
    ptr = ptr + 3;
    ptr = move_folded_white_space(ptr, size - (size_t)(ptr - start));
    if (!ptr) goto error;

    // YEAR
    parsed.tm_year = strtol(ptr, &end, 10);
    if (*end != ' ') goto error;
    if ((end - ptr != 4)) goto error;
    ptr = end + 1;
    ptr = move_folded_white_space(ptr, size - (size_t)(ptr - start));
    if (!ptr) goto error;
    parsed.tm_year = parsed.tm_year - 1900;

    // HOUR
    parsed.tm_hour = strtol(ptr, &end, 10);
    if (*end != ':') goto error;
    if ((end - ptr != 2)) goto error;
    ptr = end + 1;

    // MIN
    parsed.tm_min = strtol(ptr, &end, 10);
    if (*end != ':') goto error;
    if ((end - ptr != 2)) goto error;
    ptr = end + 1;

    // SEC
    parsed.tm_sec = strtol(ptr, &end, 10);
    if (*end != ' ') goto error;
    if ((end - ptr != 2)) goto error;
    ptr = end + 1;
    ptr = move_folded_white_space(ptr, size - (size_t)(ptr - start));
    if (!ptr) goto error;

    // check for GMT
    if (0 != strncmp(ptr, "GMT", 3)) goto error;
    ptr += 3;

    /*
     *      Somehow this MUST be set to 1 to get the correct time
     */
    parsed.tm_isdst = 0;

    /*
            time_t time_seconds = mktime(&parsed);

            ov_log_debug("time seconds %i | %i %i %i %i %i %i",
                    time_seconds,
                    parsed.tm_mday,
                    parsed.tm_mon,
                    parsed.tm_year,
                    parsed.tm_hour,
                    parsed.tm_min,
                    parsed.tm_sec);
    */
    if (next) *next = ptr;

    return parsed;
error:
    return (struct tm){0};
}

/***
        ------------------------------------------------------------------------

        Copyright 2017 German Aerospace Center DLR e.V. (GSOC)

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

        ------------------------------------------------------------------------
*//**
        @file           ov_time.c
        @author         Markus Toepfer
        @author         Michael Beer

        @date           2017-12-10

        @brief          Implementation of ov_time

        ------------------------------------------------------------------------
*/
#include "../../include/ov_time.h"
#include "ov_utils.h"
#include <inttypes.h>
#include <string.h>

char *ov_timestamp(bool micro) {

    struct timeval tv;

    size_t size = 30;
    char time_buf[30] = {0};
    char *time_utc = calloc(size, sizeof(char));

    if (0 != gettimeofday(&tv, NULL)) goto error;

    if (!micro) {
        if (!strftime(time_utc, size, "%FT%TZ", gmtime(&tv.tv_sec))) goto error;
        return time_utc;
    }

    if (!strftime(time_buf, size, "%FT%T", gmtime(&tv.tv_sec))) goto error;

    if (!snprintf(time_utc,
                  size,
                  "%s.%.6" PRIi64 "Z",
                  time_buf,
                  (int64_t)(tv.tv_usec)))
        goto error;

    return time_utc;
error:
    free(time_utc);
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_timestamp_write_to(bool micro, char *buffer, size_t size) {

    if ((NULL == buffer) || (size < 21)) return false;

    if (micro)
        if (size < 28) return false;

    struct timeval tv;
    char time_utc[25];

    if (0 != gettimeofday(&tv, NULL)) return false;

    if (!micro) {

        if (!strftime(buffer, size, "%FT%TZ", gmtime(&tv.tv_sec))) return false;
        return true;
    }

    if (!strftime(time_utc, 25, "%FT%T", gmtime(&tv.tv_sec))) return false;

    if (!snprintf(
            buffer, size, "%s.%.6" PRIi64 "Z", time_utc, (int64_t)(tv.tv_usec)))
        return false;

    return true;
}

/*----------------------------------------------------------------------------*/

char *ov_time_string(ov_time_scope_t scope) {

    struct timeval tv;

    size_t size = 30;
    char time_buf[30] = {0};
    char *time_utc = calloc(size, sizeof(char));

    if (0 != gettimeofday(&tv, NULL)) return NULL;

    switch (scope) {

        case TIME_SCOPE_YEAR:
            if (!strftime(time_utc, size, "%Y", gmtime(&tv.tv_sec))) goto error;
            break;
        case TIME_SCOPE_MONTH:
            if (!strftime(time_utc, size, "%Y-%m", gmtime(&tv.tv_sec)))
                goto error;
            break;
        case TIME_SCOPE_DAY:
            if (!strftime(time_utc, size, "%F", gmtime(&tv.tv_sec))) goto error;
            break;
        case TIME_SCOPE_HOUR:
            if (!strftime(time_utc, size, "%F %H", gmtime(&tv.tv_sec)))
                goto error;
            break;
        case TIME_SCOPE_MINUTE:
            if (!strftime(time_utc, size, "%F %R", gmtime(&tv.tv_sec)))
                goto error;
            break;
        case TIME_SCOPE_SECOND:
            if (!strftime(time_utc, size, "%F %T", gmtime(&tv.tv_sec)))
                goto error;
            break;
        case TIME_SCOPE_MILLISECOND:
            if (!strftime(time_buf, size, "%F %T", gmtime(&tv.tv_sec)))
                goto error;
            if (!snprintf(time_utc,
                          size,
                          "%s.%.3" PRIi64,
                          time_buf,
                          (int64_t)(tv.tv_usec) / 1000))
                goto error;
            break;
        case TIME_SCOPE_MICROSECOND:
            if (!strftime(time_buf, size, "%F %T", gmtime(&tv.tv_sec)))
                goto error;
            if (!snprintf(time_utc,
                          size,
                          "%s.%.6" PRIi64,
                          time_buf,
                          (int64_t)(tv.tv_usec)))
                goto error;
            break;

        default:
            goto error;
    }

    return time_utc;
error:
    free(time_utc);
    return NULL;
}

/*----------------------------------------------------------------------------*/

char *ov_timestamp_from_data(uint16_t year,
                             uint8_t month,
                             uint8_t day,
                             uint8_t hour,
                             uint8_t minute,
                             uint8_t second) {

    size_t size = 30;
    char *time_utc = calloc(size, sizeof(char));

    if (month > 12) goto error;
    if (day > 31) goto error;
    if (day == 0) goto error;
    if (month == 0) goto error;
    if (hour > 23) goto error;
    if (minute > 59) goto error;
    if (second > 59) goto error;

    if (!snprintf(time_utc,
                  size,
                  "%.04i-%.02i-%.02iT%.02i:%.02i:%.02iZ",
                  year,
                  month,
                  day,
                  hour,
                  minute,
                  second))
        goto error;

    return time_utc;
error:
    if (time_utc) free(time_utc);
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_timestamp_parse(const char *timestamp,
                        uint16_t *year,
                        uint8_t *month,
                        uint8_t *day,
                        uint8_t *hour,
                        uint8_t *minute,
                        uint8_t *second) {

    if (!timestamp || !year || !month || !day || !hour || !minute || !second)
        goto error;

    // expect something like this 2017-11-20T14:57:01Z

    if (strlen(timestamp) != 20) goto error;

    const char *t = strchr(timestamp, 'T');
    if (!t) goto error;
    if (t - timestamp != 10) goto error;

    const char *z = strchr(timestamp, 'Z');
    if (!z) goto error;
    if (z - timestamp != 19) goto error;

    const char *minus1 = strchr(timestamp, '-');
    if (!minus1) goto error;
    if (minus1 > t) goto error;

    const char *minus2 = strchr(minus1 + 1, '-');
    if (!minus2) goto error;
    if (minus2 > t) goto error;

    const char *colon1 = strchr(t, ':');
    if (!colon1) goto error;
    if (colon1 > z) goto error;

    const char *colon2 = strchr(colon1 + 1, ':');
    if (!colon2) goto error;
    if (colon2 > z) goto error;

    if (z - colon2 != 3) goto error;
    if (colon2 - colon1 != 3) goto error;
    if (t - minus2 != 3) goto error;
    if (minus2 - minus1 != 3) goto error;

    char *ptr = NULL;

    *year = (uint16_t)strtoll(timestamp, &ptr, 10);
    if (ptr != minus1) goto error;
    *month = (uint8_t)strtoll(minus1 + 1, &ptr, 10);
    if (ptr != minus2) goto error;
    *day = (uint8_t)strtoll(minus2 + 1, &ptr, 10);
    if (ptr != t) goto error;

    *hour = (uint8_t)strtoll(t + 1, &ptr, 10);
    if (ptr != colon1) goto error;
    *minute = (uint8_t)strtoll(colon1 + 1, &ptr, 10);
    if (ptr != colon2) goto error;
    *second = (uint8_t)strtoll(colon2 + 1, &ptr, 10);
    if (ptr != z) goto error;

    return true;
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_time ov_timestamp_create() {

    char *timestamp = ov_timestamp(false);
    ov_time out = ov_timestamp_from_string(timestamp);
    free(timestamp);
    return out;
}

/*----------------------------------------------------------------------------*/

bool ov_time_write_to(ov_time_scope_t scope, char *buffer, size_t size) {

    if (!buffer || size < 5) return false;

    struct timeval tv;

    if (0 != gettimeofday(&tv, NULL)) return false;

    switch (scope) {

        case TIME_SCOPE_YEAR:

            if (size < 5) return false;

            if (!strftime(buffer, size, "%Y", gmtime(&tv.tv_sec))) return false;

            break;

        case TIME_SCOPE_MONTH:

            if (size < 8) return false;

            if (!strftime(buffer, size, "%Y-%m", gmtime(&tv.tv_sec)))
                return false;
            break;

        case TIME_SCOPE_DAY:

            if (size < 11) return false;

            if (!strftime(buffer, size, "%F", gmtime(&tv.tv_sec))) return false;
            break;

        case TIME_SCOPE_HOUR:

            if (size < 14) return false;

            if (!strftime(buffer, size, "%F %H", gmtime(&tv.tv_sec)))
                return false;
            break;

        case TIME_SCOPE_MINUTE:

            if (size < 17) return false;

            if (!strftime(buffer, size, "%F %R", gmtime(&tv.tv_sec)))
                return false;
            break;

        case TIME_SCOPE_SECOND:

            if (size < 20) return false;

            if (!strftime(buffer, size, "%F %T", gmtime(&tv.tv_sec)))
                return false;
            break;

        case TIME_SCOPE_MILLISECOND:

            if (size < 24) return false;

            if (!strftime(buffer, size, "%F %T", gmtime(&tv.tv_sec)))
                return false;

            if (!snprintf(buffer + 19,
                          size,
                          ".%.3" PRIi64,
                          (int64_t)(tv.tv_usec) / 1000))
                return false;

            break;

        case TIME_SCOPE_MICROSECOND:

            if (size < 27) return false;

            if (!strftime(buffer, size, "%F %T", gmtime(&tv.tv_sec)))
                return false;

            if (!snprintf(
                    buffer + 19, size, ".%.6" PRIi64, (int64_t)(tv.tv_usec)))
                return false;

            break;

        default:
            return false;
    }

    return true;
}

/*----------------------------------------------------------------------------*/

uint64_t ov_time_get_abs_timeout_usec(uint64_t rel_timeout_usec) {

    struct timespec time_absolute;

    memset(&time_absolute, 0, sizeof(struct timespec));

    clock_gettime(CLOCK_MONOTONIC, &time_absolute);
    uint64_t abs_timeout_usec = time_absolute.tv_sec;
    abs_timeout_usec *= 1000 * 1000;
    abs_timeout_usec += time_absolute.tv_nsec / 1000;
    abs_timeout_usec += rel_timeout_usec;

    return abs_timeout_usec;
}

/*---------------------------------------------------------------------------*/

uint64_t ov_time_get_current_time_usecs() {

    struct timespec time_absolute;

    memset(&time_absolute, 0, sizeof(struct timespec));

    clock_gettime(CLOCK_MONOTONIC, &time_absolute);
    uint64_t abs_timeout_usec = time_absolute.tv_sec;
    abs_timeout_usec *= 1000 * 1000;
    abs_timeout_usec += time_absolute.tv_nsec / 1000;

    return abs_timeout_usec;
}

/*----------------------------------------------------------------------------*/

static bool time_to_string_sec_nocheck(char *target,
                                       size_t size,
                                       uint64_t timeval) {

    OV_ASSERT(0 != target);
    OV_ASSERT(0 < size);

    const time_t tv = (time_t)timeval;

    return 0 < strftime(target, size, "%Y-%m-%dT%H:%M:%SZ", gmtime(&tv));
}

/*----------------------------------------------------------------------------*/

static bool time_to_string_usec_nocheck(char *target,
                                        size_t size,
                                        uint64_t timeval) {

    OV_ASSERT(0 != target);
    OV_ASSERT(0 < size);

    const uint64_t USECS_PER_SEC = 1000 * 1000;

    time_t secs = (time_t)(timeval / USECS_PER_SEC);
    uint64_t usec = timeval - USECS_PER_SEC * secs;

    OV_ASSERT(USECS_PER_SEC * secs <= timeval);

    char buf[30] = {0};

    const size_t bufsize = sizeof(buf) / sizeof(buf[0]);

    if (0 == strftime(buf, bufsize, "%Y-%m-%dT%H:%M:%S", gmtime(&secs))) {
        return false;
    }

    return 0 < snprintf(target, size, "%s.%.6" PRIu64 "Z", buf, usec);
}

/*----------------------------------------------------------------------------*/

bool ov_time_to_string(char *target,
                       size_t size,
                       uint64_t timeval,
                       ov_time_unit unit) {

    if (0 >= unit) {
        return false;
    }

    if ((0 == target) || (0 == size)) {
        goto error;
    }

    switch (unit) {

        case SEC:

            return time_to_string_sec_nocheck(target, size, timeval);

        case USEC:

            return time_to_string_usec_nocheck(target, size, timeval);
    }

error:

    return false;
}

/*----------------------------------------------------------------------------*/

ov_time ov_timestamp_from_string(const char *timestamp) {

    ov_time out = {0};

    if (!timestamp) return out;

    ov_timestamp_parse(timestamp,
                       &out.year,
                       &out.month,
                       &out.day,
                       &out.hour,
                       &out.minute,
                       &out.second);

    return out;
}
/*----------------------------------------------------------------------------*/

char *ov_timestamp_to_string(ov_time time) {

    return ov_timestamp_from_data(
        time.year, time.month, time.day, time.hour, time.minute, time.second);
}

/*----------------------------------------------------------------------------*/

uint64_t ov_time_to_epoch(ov_time time) {

    struct tm tm;
    memset(&tm, 0, sizeof(tm));

    tm.tm_year = time.year;
    tm.tm_mon = time.month;
    tm.tm_mday = time.day;
    tm.tm_hour = time.hour;
    tm.tm_min = time.minute;
    tm.tm_sec = time.second;

    time_t timeSinceEpoch = mktime(&tm);
    return timeSinceEpoch;
}
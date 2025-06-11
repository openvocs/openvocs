/***
        ------------------------------------------------------------------------

        Copyright 2021 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_log.h
        @author         Michael Beer

        ------------------------------------------------------------------------
*/
#ifndef ov_log_ng_h
#define ov_log_ng_h

#include <stdbool.h>
#include <stddef.h>

/*----------------------------------------------------------------------------*/

typedef enum {

    OV_LOG_EMERG = 0,
    OV_LOG_ALERT = 1,
    OV_LOG_CRIT,
    OV_LOG_ERR,
    OV_LOG_WARNING,
    OV_LOG_NOTICE,
    OV_LOG_INFO,
    OV_LOG_DEBUG,
    OV_LOG_DEV,
    OV_LOG_INVALID,

} ov_log_level;

ov_log_level ov_log_level_from_string(char const *level);

/**
 * @return 0 if level unknown
 */
char const *ov_log_level_to_string(ov_log_level level);

/*----------------------------------------------------------------------------*/

/**
 * Initializes logging facility.
 * If not done, logging will still be performed, but just to stderr.
 */
bool ov_log_init();

bool ov_log_close();

/*----------------------------------------------------------------------------*/

typedef enum {

    OV_LOG_FORMAT_INVALID = -1,
    OV_LOG_TEXT = 0, // Should be the default
    OV_LOG_JSON,

} ov_log_format;

typedef struct {

    /* Flags that enable a particular logging facility or not */
    struct {
        bool systemd : 1;
    } use;

    /* Optional: stream to log to */
    int filehandle;
    ov_log_format format;

    struct {

        bool use;
        size_t messages_per_file;
        size_t max_num_files;
        char *path;

    } log_rotation;

} ov_log_output;

/*----------------------------------------------------------------------------*/

#define ov_log_configure(module, function, level, ...)                         \
    ov_log_set_output(module, function, level, (ov_log_output){__VA_ARGS__})

/**
 * Set custom logger for a file or file/function
 * @return File handle if there was a file handle associated
 */
int ov_log_set_output(char const *module_name,
                      char const *function_name,
                      ov_log_level level,
                      const ov_log_output output);

/*----------------------------------------------------------------------------*/

bool ov_log_ng(ov_log_level level,
               char const *file,
               char const *function,
               size_t line,
               char const *format,
               ...);

/*----------------------------------------------------------------------------*/

/**
 * Stop logging anything but error messages
 */
void ov_log_mute();

/**
 * After this call, everything will be logged again.
 *
 * Enable full logging again after call to `ov_log_mute` .
 * If `ov_log_mute` was not called before, has no effect.
 */
void ov_log_unmute();

/*----------------------------------------------------------------------------*/

#undef ov_log_dev
#undef ov_log_debug
#undef ov_log_info
#undef ov_log_notice
#undef ov_log_warning
#undef ov_log_error
#undef ov_log_critical
#undef ov_log_alert
#undef ov_log_emergency

#define ov_log_dev(M, ...)                                                     \
    ov_log_ng(OV_LOG_DEV, __FILE__, __FUNCTION__, __LINE__, M, ##__VA_ARGS__)

#define ov_log_debug(M, ...)                                                   \
    ov_log_ng(OV_LOG_DEBUG, __FILE__, __FUNCTION__, __LINE__, M, ##__VA_ARGS__)

#define ov_log_info(M, ...)                                                    \
    ov_log_ng(OV_LOG_INFO, __FILE__, __FUNCTION__, __LINE__, M, ##__VA_ARGS__)

#define ov_log_notice(M, ...)                                                  \
    ov_log_ng(OV_LOG_NOTICE, __FILE__, __FUNCTION__, __LINE__, M, ##__VA_ARGS__)

#define ov_log_warning(M, ...)                                                 \
    ov_log_ng(                                                                 \
        OV_LOG_WARNING, __FILE__, __FUNCTION__, __LINE__, M, ##__VA_ARGS__)

#define ov_log_error(M, ...)                                                   \
    ov_log_ng(OV_LOG_ERR, __FILE__, __FUNCTION__, __LINE__, M, ##__VA_ARGS__)

#define ov_log_critical(M, ...)                                                \
    ov_log_ng(OV_LOG_CRIT, __FILE__, __FUNCTION__, __LINE__, M, ##__VA_ARGS__)

#define ov_log_alert(M, ...)                                                   \
    ov_log_ng(OV_LOG_ALERT, __FILE__, __FUNCTION__, __LINE__, M, ##__VA_ARGS__)

#define ov_log_emergency(M, ...)                                               \
    ov_log_ng(OV_LOG_EMERG, __FILE__, __FUNCTION__, __LINE__, M, ##__VA_ARGS__)

#endif /* ov_log_ng_h */

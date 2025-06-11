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
        @file           ov_file_format_test.c
        @author         Markus Töpfer
        @author         Michael J. Beer

        @date           2021-01-02


        ------------------------------------------------------------------------
*/
#include "ov_file_format.c"
#include <ov_test/ov_test.h>

#include <ctype.h>
#include <ov_base/ov_json.h>
#include <ov_base/ov_random.h>
#include <ov_base/ov_time.h>

#include <pthread.h>

/*----------------------------------------------------------------------------*/

static bool handler_is_in_format_registry(ov_format_registry *registry,
                                          char const *type) {

    bool result = false;

    if (0 == type) return false;

    char const *test_string =
        "Geir nu garmr mjok fyr gnipahellir - festr man "
        "slitna en freki renna";

    const size_t test_string_len = strlen(test_string) + 1;

    ov_format *dummy_bare =
        ov_format_from_memory((uint8_t *)test_string, test_string_len, OV_READ);

    if (0 == dummy_bare) return false;

    ov_format *type_fmt = ov_format_as(dummy_bare, type, 0, registry);

    if (0 != type_fmt) {

        result = true;
        type_fmt = ov_format_close(type_fmt);
        dummy_bare = 0;
    }

    if (0 != dummy_bare) {

        dummy_bare = ov_format_close(dummy_bare);
    }

    OV_ASSERT(0 == type_fmt);
    OV_ASSERT(0 == dummy_bare);

    return result;
}

/*----------------------------------------------------------------------------*/

static bool handlers_in_format_registry(ov_format_registry *registry,
                                        char const *const *types,
                                        const size_t num_types) {

    for (size_t i = 0; num_types > i; ++i) {

        if (!handler_is_in_format_registry(registry, types[i])) return false;
    }

    return true;
}

/*----------------------------------------------------------------------------*/

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int check_parse_extensions() {

    ov_file_desc desc = {0};

    char *path = "/";
    testrun(!parse_extensions(NULL, NULL, 0));
    testrun(!parse_extensions(&desc, NULL, 0));
    testrun(!parse_extensions(NULL, path, 0));
    testrun(parse_extensions(&desc, path, 0));

    for (size_t i = 0; i < OV_FILE_EXT_MAX; i++) {
        for (size_t k = 0; k < OV_FILE_EXT_STRING_MAX; k++) {
            testrun(0 == desc.ext[i][k]);
        }
    }

    path = "/!";
    testrun(parse_extensions(&desc, path, 0));

    for (size_t i = 0; i < OV_FILE_EXT_MAX; i++) {
        for (size_t k = 0; k < OV_FILE_EXT_STRING_MAX; k++) {
            testrun(0 == desc.ext[i][k]);
        }
    }

    path = "x.y";
    testrun(parse_extensions(&desc, path, 0));
    testrun('y' == desc.ext[0][0]);
    testrun(0 == strncmp(desc.ext[0], "y", 1));

    path = ".";
    testrun(parse_extensions(&desc, path, 0));

    for (size_t i = 0; i < OV_FILE_EXT_MAX; i++) {
        for (size_t k = 0; k < OV_FILE_EXT_STRING_MAX; k++) {
            testrun(0 == desc.ext[i][k]);
        }
    }

    path = "..";
    testrun(parse_extensions(&desc, path, 0));

    for (size_t i = 0; i < OV_FILE_EXT_MAX; i++) {
        for (size_t k = 0; k < OV_FILE_EXT_STRING_MAX; k++) {
            testrun(0 == desc.ext[i][k]);
        }
    }

    path = "x.y.z";
    testrun(parse_extensions(&desc, path, 0));
    testrun('z' == desc.ext[0][0]);
    testrun(0 == strncmp(desc.ext[0], "z", 1));
    testrun('y' == desc.ext[1][0]);
    testrun(0 == strncmp(desc.ext[1], "y", 1));

    path = "x.one.two.three.four";
    testrun(parse_extensions(&desc, path, 0));
    testrun('f' == desc.ext[0][0]);
    testrun(0 == strncmp(desc.ext[0], "four", 4));
    testrun('t' == desc.ext[1][0]);
    testrun(0 == strncmp(desc.ext[1], "three", 5));
    testrun('t' == desc.ext[2][0]);
    testrun(0 == strncmp(desc.ext[2], "two", 3));
    testrun('o' == desc.ext[3][0]);
    testrun(0 == strncmp(desc.ext[3], "one", 3));

    for (size_t i = 4; i < OV_FILE_EXT_MAX; i++) {
        for (size_t k = 0; k < OV_FILE_EXT_STRING_MAX; k++) {
            testrun(0 == desc.ext[i][k]);
        }
    }

    // do stop at 2 dots
    path = "x.one..three.four";
    testrun(parse_extensions(&desc, path, 0));

    testrun('f' == desc.ext[0][0]);
    testrun(0 == strncmp(desc.ext[0], "four", 4));

    for (size_t i = 1; i < OV_FILE_EXT_MAX; i++) {
        for (size_t k = 0; k < OV_FILE_EXT_STRING_MAX; k++) {
            testrun(0 == desc.ext[i][k]);
        }
    }

    // do stop at 2 dots
    path = "..two.three.four";
    testrun(parse_extensions(&desc, path, 0));
    testrun('f' == desc.ext[0][0]);
    testrun(0 == strncmp(desc.ext[0], "four", 4));
    testrun('t' == desc.ext[1][0]);
    testrun(0 == strncmp(desc.ext[1], "three", 5));

    for (size_t i = 2; i < OV_FILE_EXT_MAX; i++) {
        for (size_t k = 0; k < OV_FILE_EXT_STRING_MAX; k++) {
            testrun(0 == desc.ext[i][k]);
        }
    }

    // do stop at 2 dots
    path = "/../two.three.four";
    testrun(parse_extensions(&desc, path, 0));
    testrun('f' == desc.ext[0][0]);
    testrun(0 == strncmp(desc.ext[0], "four", 4));
    testrun('t' == desc.ext[1][0]);
    testrun(0 == strncmp(desc.ext[1], "three", 5));

    for (size_t i = 2; i < OV_FILE_EXT_MAX; i++) {
        for (size_t k = 0; k < OV_FILE_EXT_STRING_MAX; k++) {
            testrun(0 == desc.ext[i][k]);
        }
    }

    // check we expect at least ONE char as filename
    path = ".one.two.three.four";
    testrun(parse_extensions(&desc, path, 0));
    testrun('f' == desc.ext[0][0]);
    testrun(0 == strncmp(desc.ext[0], "four", 4));
    testrun('t' == desc.ext[1][0]);
    testrun(0 == strncmp(desc.ext[1], "three", 5));
    testrun('t' == desc.ext[2][0]);
    testrun(0 == strncmp(desc.ext[2], "two", 3));
    testrun(0 == desc.ext[3][0]);

    for (size_t i = 4; i < OV_FILE_EXT_MAX; i++) {
        for (size_t k = 0; k < OV_FILE_EXT_STRING_MAX; k++) {
            testrun(0 == desc.ext[i][k]);
        }
    }

    // check we expect at least ONE char as filename
    // and do NOT validate the name!!!
    path = " .two.three.four";
    testrun(parse_extensions(&desc, path, 0));
    testrun('f' == desc.ext[0][0]);
    testrun(0 == strncmp(desc.ext[0], "four", 4));
    testrun('t' == desc.ext[1][0]);
    testrun(0 == strncmp(desc.ext[1], "three", 5));
    testrun('t' == desc.ext[2][0]);
    testrun(0 == strncmp(desc.ext[2], "two", 3));
    testrun(0 == desc.ext[3][0]);

    for (size_t i = 4; i < OV_FILE_EXT_MAX; i++) {
        for (size_t k = 0; k < OV_FILE_EXT_STRING_MAX; k++) {
            testrun(0 == desc.ext[i][k]);
        }
    }

    // check we parse dots only (and so support UTF8 unicode whatever)
    path = " .tw \ro.th\tree. four";
    testrun(parse_extensions(&desc, path, 0));
    testrun(' ' == desc.ext[0][0]);
    testrun(0 == strncmp(desc.ext[0], " four", 5));
    testrun('t' == desc.ext[1][0]);
    testrun(0 == strncmp(desc.ext[1], "th\tree", 6));
    testrun('t' == desc.ext[2][0]);
    testrun(0 == strncmp(desc.ext[2], "tw \ro", 5));
    testrun(0 == desc.ext[3][0]);

    for (size_t i = 4; i < OV_FILE_EXT_MAX; i++) {
        for (size_t k = 0; k < OV_FILE_EXT_STRING_MAX; k++) {
            testrun(0 == desc.ext[i][k]);
        }
    }

    path = "../file.one.two.three.four";
    testrun(parse_extensions(&desc, path, 0));
    testrun('f' == desc.ext[0][0]);
    testrun(0 == strncmp(desc.ext[0], "four", 4));
    testrun('t' == desc.ext[1][0]);
    testrun(0 == strncmp(desc.ext[1], "three", 5));
    testrun('t' == desc.ext[2][0]);
    testrun(0 == strncmp(desc.ext[2], "two", 3));
    testrun('o' == desc.ext[3][0]);
    testrun(0 == strncmp(desc.ext[3], "one", 3));

    for (size_t i = 4; i < OV_FILE_EXT_MAX; i++) {
        for (size_t k = 0; k < OV_FILE_EXT_STRING_MAX; k++) {
            testrun(0 == desc.ext[i][k]);
        }
    }

    // . as first char will be ignored
    path = "./file.one.two.three.four";
    testrun(parse_extensions(&desc, path, 0));
    testrun('f' == desc.ext[0][0]);
    testrun(0 == strncmp(desc.ext[0], "four", 4));
    testrun('t' == desc.ext[1][0]);
    testrun(0 == strncmp(desc.ext[1], "three", 5));
    testrun('t' == desc.ext[2][0]);
    testrun(0 == strncmp(desc.ext[2], "two", 3));
    testrun('o' == desc.ext[3][0]);
    testrun(0 == strncmp(desc.ext[3], "one", 3));

    for (size_t i = 4; i < OV_FILE_EXT_MAX; i++) {
        for (size_t k = 0; k < OV_FILE_EXT_STRING_MAX; k++) {
            testrun(0 == desc.ext[i][k]);
        }
    }

    // ./ will be evaluated as extension ... (no stop defined)
    path = "/./file.one.two.three.four";
    testrun(parse_extensions(&desc, path, 0));
    testrun('f' == desc.ext[0][0]);
    testrun(0 == strncmp(desc.ext[0], "four", 4));
    testrun('t' == desc.ext[1][0]);
    testrun(0 == strncmp(desc.ext[1], "three", 5));
    testrun('t' == desc.ext[2][0]);
    testrun(0 == strncmp(desc.ext[2], "two", 3));
    testrun('o' == desc.ext[3][0]);
    testrun(0 == strncmp(desc.ext[3], "one", 3));
    testrun('/' == desc.ext[4][0]);
    testrun(0 == strncmp(desc.ext[4], "/file", 5));

    for (size_t i = 5; i < OV_FILE_EXT_MAX; i++) {
        for (size_t k = 0; k < OV_FILE_EXT_STRING_MAX; k++) {
            testrun(0 == desc.ext[i][k]);
        }
    }

    // ./ will NOT be evaluated as extension ... (linux path as stop)
    path = "/./file.one.two.three.four";
    testrun(parse_extensions(&desc, path, '/'));
    testrun('f' == desc.ext[0][0]);
    testrun(0 == strncmp(desc.ext[0], "four", 4));
    testrun('t' == desc.ext[1][0]);
    testrun(0 == strncmp(desc.ext[1], "three", 5));
    testrun('t' == desc.ext[2][0]);
    testrun(0 == strncmp(desc.ext[2], "two", 3));
    testrun('o' == desc.ext[3][0]);
    testrun(0 == strncmp(desc.ext[3], "one", 3));

    for (size_t i = 4; i < OV_FILE_EXT_MAX; i++) {
        for (size_t k = 0; k < OV_FILE_EXT_STRING_MAX; k++) {
            testrun(0 == desc.ext[i][k]);
        }
    }

    // .\ will NOT be evaluated as extension ... (windows path as stop)
    path = "\\.\\file.one.two.three.four";
    testrun(parse_extensions(&desc, path, 0x5C));
    testrun('f' == desc.ext[0][0]);
    testrun(0 == strncmp(desc.ext[0], "four", 4));
    testrun('t' == desc.ext[1][0]);
    testrun(0 == strncmp(desc.ext[1], "three", 5));
    testrun('t' == desc.ext[2][0]);
    testrun(0 == strncmp(desc.ext[2], "two", 3));
    testrun('o' == desc.ext[3][0]);
    testrun(0 == strncmp(desc.ext[3], "one", 3));

    for (size_t i = 4; i < OV_FILE_EXT_MAX; i++) {
        for (size_t k = 0; k < OV_FILE_EXT_STRING_MAX; k++) {
            testrun(0 == desc.ext[i][k]);
        }
    }

    // check to lower
    path = "file.oNe.Two.thRee112.FOUR";
    testrun(parse_extensions(&desc, path, '/'));
    testrun('f' == desc.ext[0][0]);
    testrun(0 == strncmp(desc.ext[0], "four", 4));
    testrun('t' == desc.ext[1][0]);
    testrun(0 == strncmp(desc.ext[1], "three112", 8));
    testrun('t' == desc.ext[2][0]);
    testrun(0 == strncmp(desc.ext[2], "two", 3));
    testrun('o' == desc.ext[3][0]);
    testrun(0 == strncmp(desc.ext[3], "one", 3));

    for (size_t i = 4; i < OV_FILE_EXT_MAX; i++) {
        for (size_t k = 0; k < OV_FILE_EXT_STRING_MAX; k++) {
            testrun(0 == desc.ext[i][k]);
        }
    }

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_file_encoding_is_utf8() {

    char *file_ascii = ov_test_get_resource_path("ascii.file");
    char *file_utf8 = ov_test_get_resource_path("utf8.file");
    char *file_non_utf8 = ov_test_get_resource_path("non_utf8.file");

    unlink(file_ascii);
    unlink(file_utf8);
    unlink(file_non_utf8);

    size_t size = 100;
    uint8_t buf[size];
    memset(buf, 0, size);

    // write ascii file
    testrun(ov_random_bytes(buf, size));
    for (size_t i = 0; i < size; i++) {

        if (isascii(buf[i])) continue;

        buf[i] = 'A';
    }

    testrun(OV_FILE_SUCCESS == ov_file_write(file_ascii, buf, size, "w"));

    // write utf8 file
    uint8_t *buffer = buf;
    size_t len = size;
    // write at least 20 UTF8 chars
    testrun(ov_utf8_generate_random_buffer(&buffer, &len, 10));
    testrun(len < 96);
    // add some 4 byte UTF8 char, to ensure we do have some mulitbyte char
    buf[len + 1] = 0xF0; // Gothic Letter Ahsa U+10330
    buf[len + 2] = 0x90;
    buf[len + 3] = 0x8C;
    buf[len + 4] = 0xB0;

    testrun(OV_FILE_SUCCESS == ov_file_write(file_utf8, buf, size, "w"));

    // we have the valid UTF8 sequence in buf, so we make it invalid
    buf[len + 1] = 0xF0; // invalid 4 byte instead of Ahsa
    buf[len + 2] = 0xff;
    buf[len + 3] = 0x8C;
    buf[len + 4] = 0xB2;

    testrun(OV_FILE_SUCCESS == ov_file_write(file_non_utf8, buf, size, "w"));

    // test prep done

    testrun(!ov_file_encoding_is_utf8(NULL));

    // check path not file
    char *non_file_path = ov_test_get_resource_path(0);
    testrun(!ov_file_encoding_is_utf8(non_file_path));
    free(non_file_path);
    non_file_path = 0;

    // check file not found
    testrun(!ov_file_encoding_is_utf8("/not_exists_42431mfamsfaagggargagarsg"));

    testrun(ov_file_encoding_is_utf8(file_ascii));
    testrun(ov_file_encoding_is_utf8(file_utf8));
    testrun(!ov_file_encoding_is_utf8(file_non_utf8));

    unlink(file_ascii);
    unlink(file_utf8);
    unlink(file_non_utf8);

    free(file_ascii);
    free(file_utf8);
    free(file_non_utf8);

    return testrun_log_success();
}
/*----------------------------------------------------------------------------*/

static ov_buffer dummy_next_chunk(ov_format *f,
                                  size_t requested_bytes,
                                  void *data) {

    if (f || (0 == requested_bytes) || data) {
    };

    // this function does nothing but exists
    return (ov_buffer){0};
}

/*----------------------------------------------------------------------------*/

int test_ov_file_format_register() {

    ov_file_format_registry *reg = NULL;

    char const *ext[] = {"jpg", "jpeg", "JPG", "JPEG", "jPeg", "Jpg"};

    ov_file_format_parameter para =
        (ov_file_format_parameter){.name = "jpeg", .mime = "image/jpeg"};

    const ov_file_format_parameter *params = NULL;
    const ov_file_format_parameter *params2 = NULL;

    testrun(
        !ov_file_format_register(NULL, (ov_file_format_parameter){0}, 0, NULL));
    testrun(
        !ov_file_format_register(&reg, (ov_file_format_parameter){0}, 0, NULL));
    testrun(!ov_file_format_register(NULL, para, 0, NULL));

    testrun(NULL == reg);
    // create registry and set "jpeg"
    testrun(ov_file_format_register(&reg, para, 0, NULL));
    testrun(NULL != reg);
    // check registry created
    testrun(ov_file_format_registry_cast(reg));
    testrun(ov_dict_cast(reg->format));
    testrun(ov_dict_cast(reg->extensions));
    testrun(ov_dict_cast(reg->format_handler));
    testrun(1 == ov_dict_count(reg->format));
    testrun(0 == ov_dict_count(reg->extensions));

    testrun(handler_is_in_format_registry(reg->format_handler, para.name));

    params = ov_file_format_get(reg, para.name);
    testrun(params);
    testrun(0 == strcmp(params->name, para.name));
    testrun(0 == strcmp(params->mime, para.mime));

    testrun(0 == params->handler.create_data);
    testrun(0 == params->handler.free_data);
    testrun(0 == params->handler.next_chunk);
    testrun(0 == params->handler.write_chunk);

    testrun(params != &para);

    // check override
    ov_file_format_registry *backup = reg;
    testrun(ov_file_format_register(&reg, para, 0, NULL));
    testrun(NULL != reg);
    testrun(reg == backup);
    // check content
    testrun(1 == ov_dict_count(reg->format));
    testrun(0 == ov_dict_count(reg->extensions));

    testrun(handler_is_in_format_registry(reg->format_handler, para.name));

    params = ov_file_format_get(reg, para.name);
    testrun(params);
    testrun(0 == strcmp(params->name, para.name));
    testrun(0 == strcmp(params->mime, para.mime));

    testrun(0 == params->handler.create_data);
    testrun(0 == params->handler.free_data);
    testrun(0 == params->handler.next_chunk);
    testrun(0 == params->handler.write_chunk);

    testrun(params != &para);

    // check override with extensions
    testrun(ov_file_format_register(&reg, para, 1, ext));
    testrun(NULL != reg);
    testrun(reg == backup);
    // check content
    testrun(1 == ov_dict_count(reg->format));
    testrun(1 == ov_dict_count(reg->extensions));

    testrun(handler_is_in_format_registry(reg->format_handler, para.name));

    params = ov_file_format_get(reg, para.name);
    testrun(params);
    testrun(0 == strcmp(params->name, para.name));
    testrun(0 == strcmp(params->mime, para.mime));
    params2 = ov_file_format_get_ext(reg, ext[0]);
    testrun(params == params2);
    testrun(0 == strcmp(params->name, para.name));
    testrun(0 == strcmp(params->mime, para.mime));

    // check override with multiple extensions
    testrun(ov_file_format_register(&reg, para, 2, ext));
    testrun(NULL != reg);
    testrun(reg == backup);
    // check content
    testrun(1 == ov_dict_count(reg->format));
    testrun(2 == ov_dict_count(reg->extensions));

    testrun(handler_is_in_format_registry(reg->format_handler, para.name));

    params = ov_file_format_get(reg, para.name);
    testrun(params);
    testrun(0 == strcmp(params->name, para.name));
    testrun(0 == strcmp(params->mime, para.mime));
    params2 = ov_file_format_get_ext(reg, ext[0]);
    testrun(params == params2);
    params2 = ov_file_format_get_ext(reg, ext[1]);
    testrun(params == params2);

    // check override with multiple extensions (case ignore)
    testrun(ov_file_format_register(&reg, para, 4, ext));
    testrun(NULL != reg);
    testrun(reg == backup);
    // check content
    testrun(1 == ov_dict_count(reg->format));
    testrun(2 == ov_dict_count(reg->extensions));
    testrun(handler_is_in_format_registry(reg->format_handler, para.name));
    params = ov_file_format_get(reg, para.name);
    testrun(params);
    testrun(0 == strcmp(params->name, para.name));
    testrun(0 == strcmp(params->mime, para.mime));
    params2 = ov_file_format_get_ext(reg, ext[0]);
    testrun(params == params2);
    params2 = ov_file_format_get_ext(reg, ext[1]);
    testrun(params == params2);

    // check override with multiple extensions (case ignore)
    testrun(ov_file_format_register(&reg, para, 6, ext));
    testrun(NULL != reg);
    testrun(reg == backup);
    // check content
    testrun(1 == ov_dict_count(reg->format));
    testrun(2 == ov_dict_count(reg->extensions));
    testrun(handler_is_in_format_registry(reg->format_handler, para.name));
    params = ov_file_format_get(reg, para.name);
    testrun(params);
    testrun(0 == strcmp(params->name, para.name));
    testrun(0 == strcmp(params->mime, para.mime));
    params2 = ov_file_format_get_ext(reg, ext[0]);
    testrun(params == params2);
    params2 = ov_file_format_get_ext(reg, ext[1]);
    testrun(params == params2);

    para.handler = (ov_format_handler){.next_chunk = dummy_next_chunk};

    // override with some actual ov_format handler
    testrun(ov_file_format_register(&reg, para, 6, ext));
    testrun(NULL != reg);
    testrun(reg == backup);
    // check content
    testrun(1 == ov_dict_count(reg->format));
    testrun(2 == ov_dict_count(reg->extensions));
    testrun(handler_is_in_format_registry(reg->format_handler, para.name));
    params = ov_file_format_get(reg, para.name);
    testrun(params);
    testrun(0 == strcmp(params->name, para.name));
    testrun(0 == strcmp(params->mime, para.mime));
    params2 = ov_file_format_get_ext(reg, ext[0]);
    testrun(params == params2);
    params2 = ov_file_format_get_ext(reg, ext[1]);
    testrun(params == params2);

    // try to overide with handler set
    testrun(ov_file_format_register(&reg, para, 6, ext));
    // check content
    testrun(1 == ov_dict_count(reg->format));
    testrun(2 == ov_dict_count(reg->extensions));

    testrun(handler_is_in_format_registry(reg->format_handler, para.name));

    params = ov_file_format_get(reg, para.name);
    testrun(params);
    testrun(0 == strcmp(params->name, para.name));
    testrun(0 == strcmp(params->mime, para.mime));
    params2 = ov_file_format_get_ext(reg, ext[0]);
    testrun(params == params2);
    params2 = ov_file_format_get_ext(reg, ext[1]);
    testrun(params == params2);

    // set extensions to different name
    strcpy(para.name, "no jpeg");
    testrun(ov_file_format_register(&reg, para, 6, ext));
    // check content
    testrun(2 == ov_dict_count(reg->format));
    testrun(2 == ov_dict_count(reg->extensions));

    char const *format_types_present[] = {"jpeg", "no jpeg"};

    testrun(handlers_in_format_registry(
        reg->format_handler, format_types_present, 2));

    params = ov_file_format_get(reg, para.name);
    testrun(params);
    testrun(0 == strcmp(params->name, para.name));
    testrun(0 == strcmp(params->mime, para.mime));
    params2 = ov_file_format_get_ext(reg, ext[0]);
    testrun(params == params2);
    params2 = ov_file_format_get_ext(reg, ext[1]);
    testrun(params == params2);
    params = ov_file_format_get(reg, "jpeg");
    testrun(params);
    testrun(params != params2);

    testrun(ov_file_format_free_registry(&reg));
    testrun(NULL == reg);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_file_format_free_registry() {

    ov_file_format_registry *reg = NULL;

    char const *ext[] = {"jpg", "jpeg", "JPG", "JPEG", "jPeg", "Jpg"};

    ov_file_format_parameter para =
        (ov_file_format_parameter){.name = "jpeg", .mime = "image/jpeg"};

    testrun(NULL == reg);
    // create registry and set "jpeg"
    testrun(ov_file_format_register(&reg, para, 6, ext));

    testrun(ov_file_format_free_registry(NULL));
    testrun(reg);
    testrun(ov_file_format_free_registry(&reg));
    testrun(reg == NULL);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_file_desc_from_path() {

    size_t size = 100;
    uint8_t buf[size];
    memset(buf, 0, size);
    testrun(ov_random_bytes(buf, size));

    size_t min = 10;

    char *files[] = {ov_test_get_resource_path("file.1"),
                     ov_test_get_resource_path("file.2"),
                     ov_test_get_resource_path("file.x.y"),
                     ov_test_get_resource_path("file.a.b.c.d"),
                     ov_test_get_resource_path("file.json.zip.1"),
                     ov_test_get_resource_path("file.jG.WHAT1.eVer")};

    size_t items = sizeof(files) / sizeof(files[0]);

    for (size_t i = 0; i < items; i++) {

        unlink(files[i]);
        testrun(OV_FILE_SUCCESS == ov_file_write(files[i], buf, min + i, "w"));
    }

    ov_file_desc desc[items];
    memset(desc, 0, items * sizeof(ov_file_desc));

    for (size_t i = 0; i < items; i++) {

        desc[i] = ov_file_desc_from_path(files[i]);
        testrun(desc[i].bytes == (ssize_t)(min + i));
    }

    // check extensions parsed
    testrun(0 != desc[0].ext[0][0]);
    testrun(0 == desc[0].ext[1][0]);
    testrun(0 == memcmp(desc[0].ext[0], "1", 1));

    testrun(0 != desc[1].ext[0][0]);
    testrun(0 == desc[1].ext[1][0]);
    testrun(0 == memcmp(desc[1].ext[0], "2", 1));

    testrun(0 != desc[2].ext[0][0]);
    testrun(0 != desc[2].ext[1][0]);
    testrun(0 == desc[2].ext[2][0]);
    testrun(0 == memcmp(desc[2].ext[0], "y", 1));
    testrun(0 == memcmp(desc[2].ext[1], "x", 1));

    testrun(0 != desc[3].ext[0][0]);
    testrun(0 != desc[3].ext[1][0]);
    testrun(0 != desc[3].ext[2][0]);
    testrun(0 != desc[3].ext[3][0]);
    testrun(0 == desc[3].ext[4][0]);
    testrun(0 == memcmp(desc[3].ext[0], "d", 1));
    testrun(0 == memcmp(desc[3].ext[1], "c", 1));
    testrun(0 == memcmp(desc[3].ext[2], "b", 1));
    testrun(0 == memcmp(desc[3].ext[3], "a", 1));

    testrun(0 != desc[4].ext[0][0]);
    testrun(0 != desc[4].ext[1][0]);
    testrun(0 != desc[4].ext[2][0]);
    testrun(0 == desc[4].ext[3][0]);
    testrun(0 == memcmp(desc[4].ext[2], "json", 4));
    testrun(0 == memcmp(desc[4].ext[1], "zip", 3));
    testrun(0 == memcmp(desc[4].ext[0], "1", 1));

    // check to parse to small letters
    testrun(0 != desc[5].ext[0][0]);
    testrun(0 != desc[5].ext[1][0]);
    testrun(0 != desc[5].ext[2][0]);
    testrun(0 == desc[5].ext[3][0]);
    testrun(0 == memcmp(desc[5].ext[2], "jg", 2));
    testrun(0 == memcmp(desc[5].ext[1], "what1", 5));
    testrun(0 == memcmp(desc[5].ext[0], "ever", 4));

    char *non_file_path = ov_test_get_resource_path(0);
    ov_file_desc x = ov_file_desc_from_path(non_file_path);
    free(non_file_path);
    non_file_path = 0;
    testrun(-1 == x.bytes);

    x = ov_file_desc_from_path(
        "/dgafsfsdffwer2r3433qewa234/"
        "not_exists_42431mfamsfaagggargagarsg");
    testrun(-1 == x.bytes);

    for (size_t i = 0; i < items; i++) {
        unlink(files[i]);
        free(files[i]);
    }

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_file_format_get_desc() {

    size_t size = 100;
    uint8_t buf[size];
    memset(buf, 0, size);
    testrun(ov_random_bytes(buf, size));

    ssize_t min = 10;

    char *files[] = {ov_test_get_resource_path("file.a"),
                     ov_test_get_resource_path("file.b"),
                     ov_test_get_resource_path("file.a.b"),
                     ov_test_get_resource_path("file.a.b.c")};

    size_t items = sizeof(files) / sizeof(files[0]);

    for (size_t i = 0; i < items; i++) {

        unlink(files[i]);
        testrun(OV_FILE_SUCCESS == ov_file_write(files[i], buf, min, "w"));
    }

    ov_file_format_registry *reg = NULL;

    char const *ext_b[] = {"b"};
    ov_file_format_parameter para_b = (ov_file_format_parameter){

        .name = "b", .mime = "b_mime"};

    char const *ext_c[] = {"c"};
    ov_file_format_parameter para_c = (ov_file_format_parameter){

        .name = "c", .mime = "c_mime"};

    testrun(ov_file_format_register(&reg, para_b, 1, ext_b));
    testrun(reg);

    ov_file_format_desc format = ov_file_format_get_desc(NULL, NULL);
    testrun(-1 == format.desc.bytes);
    format = ov_file_format_get_desc(reg, NULL);
    testrun(-1 == format.desc.bytes);

    // work without some registry for mime data
    format = ov_file_format_get_desc(NULL, files[0]);
    testrun(-1 != format.desc.bytes);
    testrun(min == format.desc.bytes);
    testrun(0 != format.desc.ext[0][0]);
    testrun(0 == format.desc.ext[1][0]);
    testrun(0 == memcmp(format.desc.ext[0], "a", 1));
    testrun(0 == format.mime[0]);

    // mime type not enabled
    format = ov_file_format_get_desc(reg, files[0]);
    testrun(min == format.desc.bytes);
    testrun(0 != format.desc.ext[0][0]);
    testrun(0 == format.desc.ext[1][0]);
    testrun(0 == memcmp(format.desc.ext[0], "a", 1));
    testrun(0 == format.mime[0]);

    // mime type enabled
    format = ov_file_format_get_desc(reg, files[1]);
    testrun(min == format.desc.bytes);
    testrun(0 != format.desc.ext[0][0]);
    testrun(0 == format.desc.ext[1][0]);
    testrun(0 == memcmp(format.desc.ext[0], "b", 1));
    testrun(0 == strcmp(format.mime, "b_mime"));

    // mime type not enabled
    format = ov_file_format_get_desc(reg, files[3]);
    testrun(min == format.desc.bytes);
    testrun(0 != format.desc.ext[0][0]);
    testrun(0 != format.desc.ext[1][0]);
    testrun(0 != format.desc.ext[2][0]);
    testrun(0 == format.desc.ext[3][0]);
    testrun(0 == memcmp(format.desc.ext[0], "c", 1));
    testrun(0 == format.mime[0]);

    testrun(ov_file_format_register(&reg, para_c, 1, ext_c));

    // mime type enabled
    format = ov_file_format_get_desc(reg, files[3]);
    testrun(min == format.desc.bytes);
    testrun(0 != format.desc.ext[0][0]);
    testrun(0 != format.desc.ext[1][0]);
    testrun(0 != format.desc.ext[2][0]);
    testrun(0 == format.desc.ext[3][0]);
    testrun(0 == memcmp(format.desc.ext[0], "c", 1));
    testrun(0 == strcmp(format.mime, "c_mime"));

    // mime type of last ext applied
    format = ov_file_format_get_desc(reg, files[3]);
    testrun(min == format.desc.bytes);
    testrun(0 != format.desc.ext[0][0]);
    testrun(0 != format.desc.ext[1][0]);
    testrun(0 != format.desc.ext[2][0]);
    testrun(0 == format.desc.ext[3][0]);
    testrun(0 == memcmp(format.desc.ext[0], "c", 1));
    testrun(0 == memcmp(format.desc.ext[1], "b", 1));
    testrun(0 == memcmp(format.desc.ext[2], "a", 1));
    testrun(0 == strcmp(format.mime, "c_mime"));

    testrun(ov_file_format_free_registry(&reg));

    for (size_t i = 0; i < items; i++) {
        unlink(files[i]);
        free(files[i]);
    }

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_file_format_get() {

    ov_file_format_registry *reg = NULL;

    char const *ext[][1] = {"a", "b", "c"};

    ov_file_format_parameter para[] = {
        (ov_file_format_parameter){.name = "a", .mime = "a_mime"},
        (ov_file_format_parameter){.name = "b", .mime = "b_mime"},
        (ov_file_format_parameter){.name = "c", .mime = "c_mime"}};

    for (size_t i = 0; i < 3; i++) {
        testrun(ov_file_format_register(&reg, para[i], 1, ext[i]));
    }

    testrun(reg);

    testrun(!ov_file_format_get(NULL, NULL));
    testrun(!ov_file_format_get(reg, NULL));
    testrun(!ov_file_format_get(NULL, "a"));

    for (size_t i = 0; i < 3; i++) {
        testrun(ov_file_format_get(reg, para[i].name));
    }

    testrun(!ov_file_format_get(reg, "d"));

    testrun(ov_file_format_free_registry(&reg));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_file_format_get_ext() {

    ov_file_format_registry *reg = NULL;

    char const *ext[][1] = {"a", "b", "c"};

    ov_file_format_parameter para[] = {
        (ov_file_format_parameter){.name = "a", .mime = "a_mime"},
        (ov_file_format_parameter){.name = "b", .mime = "b_mime"},
        (ov_file_format_parameter){.name = "c", .mime = "c_mime"}};

    for (size_t i = 0; i < 3; i++) {
        testrun(ov_file_format_register(&reg, para[i], 1, ext[i]));
    }

    testrun(reg);

    testrun(!ov_file_format_get_ext(NULL, NULL));
    testrun(!ov_file_format_get_ext(reg, NULL));
    testrun(!ov_file_format_get_ext(NULL, "a"));

    for (size_t i = 0; i < 3; i++) {
        testrun(ov_file_format_get_ext(reg, ext[i][0]));
    }

    testrun(!ov_file_format_get_ext(reg, "d"));

    testrun(ov_file_format_free_registry(&reg));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

struct container {

    uint64_t stop_time;
    ov_file_format_registry *registry;

    ssize_t bytes;
    char *files[4];
    char *mime[4];
};

/*----------------------------------------------------------------------------*/

static void *thread_test_get_desc(void *arg) {

    if (!arg) goto error;

    //    pthread_t id = pthread_self();

    struct container *container = (struct container *)arg;

    uint64_t current = ov_time_get_current_time_usecs();
    uint32_t random = 0;

    while (current < container->stop_time) {

        random = ov_random_uint32() % 4;

        ov_file_format_desc format = ov_file_format_get_desc(
            container->registry, container->files[random]);
        /*
                fprintf(
                    stdout, "%li - %i|%s\n", (long)id, random,
           container->files[random]);
        */
        OV_ASSERT(format.desc.bytes == container->bytes);
        OV_ASSERT(0 == strcmp(format.mime, container->mime[random]));

        /* We do not perform the ext check here */

        current = ov_time_get_current_time_usecs();
    }

error:
    // NOTE using pthread_exit creates some valgrind unclean state,
    // as processing within the thread is done anyway, we just return here
    return NULL;
}

/*----------------------------------------------------------------------------*/

int check_parallel_requests() {

    /*
     *  This test is to verify threadsafe use of ov_file_format_get_desc
     *
     *  In principle we just spawn some threads and let the function
     *  execute in each thread.
     *
     *  This will check parallel file read over ov_file_desc_from_path,
     *  as well as parallel access of the extension dict of the registry.
     */

    size_t size = 100;
    uint8_t buf[size];
    memset(buf, 0, size);
    testrun(ov_random_bytes(buf, size));

    ssize_t min = 10;

    struct container container = (struct container){

        .files = {ov_test_get_resource_path("file.a"),
                  ov_test_get_resource_path("file.b"),
                  ov_test_get_resource_path("file.a.b"),
                  ov_test_get_resource_path("file.a.b.c")},

        .mime = {"a_mime", "b_mime", "b_mime", "c_mime"},

        .bytes = min,

    };

    char const *ext[][1] = {"a", "b", "c"};

    for (size_t i = 0; i < 4; i++) {

        unlink(container.files[i]);
        testrun(OV_FILE_SUCCESS ==
                ov_file_write(container.files[i], buf, min, "w"));
    }

    ov_file_format_registry *reg = NULL;

    testrun(ov_file_format_register(&reg,
                                    (ov_file_format_parameter){
                                        .name = "a",
                                        .mime = "a_mime",
                                    },
                                    1,
                                    ext[0]));

    testrun(ov_file_format_register(&reg,
                                    (ov_file_format_parameter){
                                        .name = "b",
                                        .mime = "b_mime",
                                    },
                                    1,
                                    ext[1]));

    testrun(ov_file_format_register(&reg,
                                    (ov_file_format_parameter){
                                        .name = "c",
                                        .mime = "c_mime",
                                    },
                                    1,
                                    ext[2]));

    /* <-- env prepared */

    uint64_t testtime = 5 * 1000 * 1000;

    container.registry = reg;
    container.stop_time = ov_time_get_current_time_usecs() + testtime;

    pthread_t threads[10] = {0};

    for (size_t i = 0; i < 10; i++) {

        testrun(0 == pthread_create(
                         &threads[i], NULL, thread_test_get_desc, &container));
    }

    for (size_t i = 0; i < 10; i++) {

        testrun(0 == pthread_join(threads[i], NULL));
    }

    /* cleanup --> */

    testrun(ov_file_format_free_registry(&reg));

    for (size_t i = 0; i < 4; i++) {
        unlink(container.files[i]);
        free(container.files[i]);
    }

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_file_format_register_values_to_json() {

    ov_json_value *out = NULL;
    ov_json_value *arr = NULL;

    testrun(!ov_file_format_register_values_to_json(
        (ov_file_format_parameter){0}, 0, NULL));

    testrun(!ov_file_format_register_values_to_json(
        (ov_file_format_parameter){0}, 0, NULL));

    testrun(!ov_file_format_register_values_to_json(
        (ov_file_format_parameter){0}, 1, NULL));

    testrun(!ov_file_format_register_values_to_json(
        (ov_file_format_parameter){0}, 0, (char const *[]){"1"}));

    // check min vaid input
    out = ov_file_format_register_values_to_json(
        (ov_file_format_parameter){0}, 1, (char const *[]){"1"});

    testrun(out);
    testrun(1 == ov_json_object_count(out));
    arr = ov_json_object_get(out, OV_KEY_EXTENSION);
    testrun(1 == ov_json_array_count(arr)) testrun(
        0 == strcmp("1", ov_json_string_get(ov_json_array_get(arr, 1))));
    out = ov_json_value_free(out);

    out = ov_file_format_register_values_to_json(
        (ov_file_format_parameter){0}, 3, (char const *[]){"1", "2", "3"});

    testrun(out);
    testrun(1 == ov_json_object_count(out));
    arr = ov_json_object_get(out, OV_KEY_EXTENSION);
    testrun(3 == ov_json_array_count(arr)) testrun(
        0 == strcmp("1", ov_json_string_get(ov_json_array_get(arr, 1))));
    testrun(0 == strcmp("2", ov_json_string_get(ov_json_array_get(arr, 2))));
    testrun(0 == strcmp("3", ov_json_string_get(ov_json_array_get(arr, 3))));
    out = ov_json_value_free(out);

    out = ov_file_format_register_values_to_json(
        (ov_file_format_parameter){.name = "name", .mime = "mime"},
        3,
        (char const *[]){"1", "2", "3"});

    testrun(out);
    testrun(2 == ov_json_object_count(out));
    testrun(0 ==
            strcmp("mime",
                   ov_json_string_get(ov_json_object_get(out, OV_KEY_MIME))));
    arr = ov_json_object_get(out, OV_KEY_EXTENSION);
    testrun(3 == ov_json_array_count(arr)) testrun(
        0 == strcmp("1", ov_json_string_get(ov_json_array_get(arr, 1))));
    testrun(0 == strcmp("2", ov_json_string_get(ov_json_array_get(arr, 2))));
    testrun(0 == strcmp("3", ov_json_string_get(ov_json_array_get(arr, 3))));
    out = ov_json_value_free(out);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_file_format_register_values_from_json() {

    ov_json_value *val = NULL;
    ov_json_value *arr = NULL;

    val = ov_file_format_register_values_to_json(
        (ov_file_format_parameter){.name = "name", .mime = "mime"},
        3,
        (char const *[]){"1", "2", "3"});

    testrun(val);
    testrun(2 == ov_json_object_count(val));
    testrun(0 ==
            strcmp("mime",
                   ov_json_string_get(ov_json_object_get(val, OV_KEY_MIME))));
    arr = ov_json_object_get(val, OV_KEY_EXTENSION);
    testrun(3 == ov_json_array_count(arr)) testrun(
        0 == strcmp("1", ov_json_string_get(ov_json_array_get(arr, 1))));
    testrun(0 == strcmp("2", ov_json_string_get(ov_json_array_get(arr, 2))));
    testrun(0 == strcmp("3", ov_json_string_get(ov_json_array_get(arr, 3))));

    ov_file_format_parameter para = {0};
    size_t size = OV_FILE_EXT_MAX;
    size_t p_size = size;
    char *array[OV_FILE_EXT_MAX];
    memset(array, 0, sizeof(array));

    testrun(!ov_file_format_register_values_from_json(
        NULL, NULL, NULL, NULL, NULL));
    testrun(!ov_file_format_register_values_from_json(
        NULL, NULL, &para, &p_size, array));
    testrun(!ov_file_format_register_values_from_json(
        val, NULL, NULL, &p_size, array));
    testrun(!ov_file_format_register_values_from_json(
        val, NULL, &para, NULL, array));
    testrun(!ov_file_format_register_values_from_json(
        val, NULL, &para, &p_size, NULL));

    testrun(ov_file_format_register_values_from_json(
        val, NULL, &para, &p_size, array));
    testrun(p_size == 3);
    testrun(0 == strcmp(array[0], "1"));
    testrun(0 == strcmp(array[1], "2"));
    testrun(0 == strcmp(array[2], "3"));
    testrun(0 == strcmp(para.mime, "mime"));
    testrun(0 == para.name[0]);

    p_size = size;
    testrun(ov_file_format_register_values_from_json(
        val, "x", &para, &p_size, array));
    testrun(p_size == 3);
    testrun(0 == strcmp(array[0], "1"));
    testrun(0 == strcmp(array[1], "2"));
    testrun(0 == strcmp(array[2], "3"));
    testrun(0 == strcmp(para.mime, "mime"));
    testrun(0 == strcmp(para.name, "x"));

    p_size = 1;
    testrun(!ov_file_format_register_values_from_json(
        val, "x", &para, &p_size, array));
    p_size = 2;
    testrun(!ov_file_format_register_values_from_json(
        val, "x", &para, &p_size, array));
    p_size = 3;
    testrun(ov_file_format_register_values_from_json(
        val, "new", &para, &p_size, array));
    testrun(p_size == 3);
    testrun(0 == strcmp(array[0], "1"));
    testrun(0 == strcmp(array[1], "2"));
    testrun(0 == strcmp(array[2], "3"));
    testrun(0 == strcmp(para.mime, "mime"));
    testrun(0 == strcmp(para.name, "new"));

    // check as integrated object
    ov_json_value *obj = ov_json_object();
    testrun(ov_json_object_set(obj, "name", val));
    p_size = size;
    testrun(!ov_file_format_register_values_from_json(
        obj, "other_name", &para, &p_size, array));
    testrun(ov_file_format_register_values_from_json(
        obj, "name", &para, &p_size, array));
    testrun(p_size == 3);
    testrun(0 == strcmp(array[0], "1"));
    testrun(0 == strcmp(array[1], "2"));
    testrun(0 == strcmp(array[2], "3"));
    testrun(0 == strcmp(para.mime, "mime"));
    testrun(0 == strcmp(para.name, "name"));

    // check without mime
    testrun(ov_json_object_del(val, OV_KEY_MIME));
    testrun(!ov_file_format_register_values_from_json(
        val, "name", &para, &p_size, array));
    testrun(!ov_file_format_register_values_from_json(
        obj, "name", &para, &p_size, array));

    ov_json_value *mime = ov_json_string("image/jpeg");
    testrun(ov_json_object_set(val, OV_KEY_MIME, mime));
    testrun(ov_file_format_register_values_from_json(
        obj, "name", &para, &p_size, array));
    testrun(p_size == 3);
    testrun(0 == strcmp(array[0], "1"));
    testrun(0 == strcmp(array[1], "2"));
    testrun(0 == strcmp(array[2], "3"));
    testrun(0 == strcmp(para.mime, "image/jpeg"));
    testrun(0 == strcmp(para.name, "name"));

    memset(array, 0, sizeof(array));
    testrun(ov_json_array_clear(ov_json_object_get(val, OV_KEY_EXTENSION)));
    testrun(0 ==
            ov_json_array_count(ov_json_object_get(val, OV_KEY_EXTENSION)));

    testrun(ov_file_format_register_values_from_json(
        obj, "name", &para, &p_size, array));
    testrun(p_size == 0);
    testrun(0 == (array[0]));
    testrun(0 == (array[1]));
    testrun(0 == (array[2]));
    testrun(0 == strcmp(para.mime, "image/jpeg"));
    testrun(0 == strcmp(para.name, "name"));

    obj = ov_json_value_free(obj);
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_file_format_register_from_json_from_path() {

    char *dir = ov_test_get_resource_path("mime");

    char *files[] = {ov_test_get_resource_path("mime/ogg.json"),
                     ov_test_get_resource_path("mime/jpeg.json"),
                     ov_test_get_resource_path("mime/html.json"),
                     ov_test_get_resource_path("mime/bmp.mime"),
                     ov_test_get_resource_path("mime/png.mime"),
                     ov_test_get_resource_path("mime/pdf"),
                     ov_test_get_resource_path("mime/truetype"),
                     ov_test_get_resource_path("mime/invalid.json")};

    size_t items = sizeof(files) / sizeof(files[0]);

    for (size_t i = 0; i < items; i++) {

        unlink(files[i]);
    }

    rmdir(dir);
    testrun(0 == mkdir(dir, 0700));

    char *content[] = {

        "{"
        "\"ogg audio\":{"
        "\"mime\": \"audio/ogg\","
        "\"extension\":["
        "\"ogg\","
        "\"oga\""
        "]"
        "},"
        "\"ogg video\":{"
        "\"mime\": \"video/ogg\","
        "\"extension\":["
        "\"ogv\""
        "]"
        "},"
        "\"ogg app\":{"
        "\"mime\": \"application/ogg\","
        "\"extension\":["
        "\"ogx\""
        "]"
        "}"
        "}",

        "{"
        "\"jpeg\":{"
        "\"mime\": \"image/jpeg\","
        "\"extension\":["
        "\"jpg\","
        "\"jpeg\""
        "]"
        "}"
        "}",

        "{"
        "\"html\":{"
        "\"mime\": \"text/html\","
        "\"extension\":["
        "\"htm\","
        "\"html\""
        "]"
        "}"
        "}",

        "{"
        "\"bmp\":{"
        "\"mime\": \"image/bmp\","
        "\"extension\":["
        "\"bmp\""
        "]"
        "}"
        "}",

        "{"
        "\"png\":{"
        "\"mime\": \"image/png\","
        "\"extension\":["
        "\"png\""
        "]"
        "}"
        "}",

        "{"
        "\"pdf\":{"
        "\"mime\": \"application/pdf\","
        "\"extension\":["
        "\"pdf\""
        "]"
        "}"
        "}",

        "{"
        "\"truetype\":{"
        "\"mime\": \"font/ttf\","
        "\"extension\":["
        "\"ttf\""
        "]"
        "}"
        "}",

        "{"
        "\"invalid\":null"
        "}"

    };

    // we do not write the invalid file yet
    for (size_t i = 0; i < items - 1; i++) {

        testrun(OV_FILE_SUCCESS ==
                ov_file_write(
                    files[i], (uint8_t *)content[i], strlen(content[i]), "w"));
    }

    ov_file_format_registry *reg = NULL;

    testrun(!ov_file_format_register_from_json_from_path(NULL, NULL, NULL));
    testrun(!ov_file_format_register_from_json_from_path(&reg, NULL, NULL));
    testrun(!ov_file_format_register_from_json_from_path(NULL, dir, NULL));

    // load all files from dir (NOTE invalid not written yet!)
    testrun(ov_file_format_register_from_json_from_path(&reg, dir, NULL));
    // check registry
    testrun(ov_file_format_get(reg, "ogg audio"));
    testrun(ov_file_format_get(reg, "ogg video"));
    testrun(ov_file_format_get(reg, "ogg app"));
    testrun(ov_file_format_get(reg, "jpeg"));
    testrun(ov_file_format_get(reg, "html"));
    testrun(ov_file_format_get(reg, "bmp"));
    testrun(ov_file_format_get(reg, "png"));
    testrun(ov_file_format_get(reg, "pdf"));
    testrun(ov_file_format_get(reg, "truetype"));
    testrun(ov_file_format_free_registry(&reg));

    // load all json extensions
    testrun(ov_file_format_register_from_json_from_path(&reg, dir, "json"));
    // check registry
    testrun(ov_file_format_get(reg, "ogg audio"));
    testrun(ov_file_format_get(reg, "ogg video"));
    testrun(ov_file_format_get(reg, "ogg app"));
    testrun(ov_file_format_get(reg, "jpeg"));
    testrun(ov_file_format_get(reg, "html"));
    testrun(!ov_file_format_get(reg, "bmp"));
    testrun(!ov_file_format_get(reg, "png"));
    testrun(!ov_file_format_get(reg, "pdf"));
    testrun(!ov_file_format_get(reg, "truetype"));
    testrun(ov_file_format_free_registry(&reg));

    // load all mime extensions
    testrun(ov_file_format_register_from_json_from_path(&reg, dir, "mime"));
    // check registry
    testrun(!ov_file_format_get(reg, "ogg audio"));
    testrun(!ov_file_format_get(reg, "ogg video"));
    testrun(!ov_file_format_get(reg, "ogg app"));
    testrun(!ov_file_format_get(reg, "jpeg"));
    testrun(!ov_file_format_get(reg, "html"));
    testrun(ov_file_format_get(reg, "bmp"));
    testrun(ov_file_format_get(reg, "png"));
    testrun(!ov_file_format_get(reg, "pdf"));
    testrun(!ov_file_format_get(reg, "truetype"));

    // load additonal json extensions
    testrun(ov_file_format_register_from_json_from_path(&reg, dir, "json"));
    // check registry
    testrun(ov_file_format_get(reg, "ogg audio"));
    testrun(ov_file_format_get(reg, "ogg video"));
    testrun(ov_file_format_get(reg, "ogg app"));
    testrun(ov_file_format_get(reg, "jpeg"));
    testrun(ov_file_format_get(reg, "html"));
    testrun(ov_file_format_get(reg, "bmp"));
    testrun(ov_file_format_get(reg, "png"));
    testrun(!ov_file_format_get(reg, "pdf"));
    testrun(!ov_file_format_get(reg, "truetype"));

    // reload all (override mode)
    testrun(ov_file_format_register_from_json_from_path(&reg, dir, NULL));
    // check registry
    testrun(ov_file_format_get(reg, "ogg audio"));
    testrun(ov_file_format_get(reg, "ogg video"));
    testrun(ov_file_format_get(reg, "ogg app"));
    testrun(ov_file_format_get(reg, "jpeg"));
    testrun(ov_file_format_get(reg, "html"));
    testrun(ov_file_format_get(reg, "bmp"));
    testrun(ov_file_format_get(reg, "png"));
    testrun(ov_file_format_get(reg, "pdf"));
    testrun(ov_file_format_get(reg, "truetype"));

    testrun(ov_file_format_free_registry(&reg));

    // write invalid.json
    testrun(OV_FILE_SUCCESS == ov_file_write(files[items - 1],
                                             (uint8_t *)content[items - 1],
                                             strlen(content[items - 1]),
                                             "w"));

    // load all mime extensions
    testrun(ov_file_format_register_from_json_from_path(&reg, dir, "mime"));
    // check registry
    testrun(!ov_file_format_get(reg, "ogg audio"));
    testrun(!ov_file_format_get(reg, "ogg video"));
    testrun(!ov_file_format_get(reg, "ogg app"));
    testrun(!ov_file_format_get(reg, "jpeg"));
    testrun(!ov_file_format_get(reg, "html"));
    testrun(ov_file_format_get(reg, "bmp"));
    testrun(ov_file_format_get(reg, "png"));
    testrun(!ov_file_format_get(reg, "pdf"));
    testrun(!ov_file_format_get(reg, "truetype"));

    // load additonal json extensions
    testrun(!ov_file_format_register_from_json_from_path(&reg, dir, "json"));
    // check registry
    testrun(!ov_file_format_get(reg, "ogg audio"));
    testrun(!ov_file_format_get(reg, "ogg video"));
    testrun(!ov_file_format_get(reg, "ogg app"));
    testrun(!ov_file_format_get(reg, "jpeg"));
    testrun(!ov_file_format_get(reg, "html"));
    testrun(ov_file_format_get(reg, "bmp"));
    testrun(ov_file_format_get(reg, "png"));
    testrun(!ov_file_format_get(reg, "pdf"));
    testrun(!ov_file_format_get(reg, "truetype"));
    testrun(ov_file_format_free_registry(&reg));

    // try to load without extension
    testrun(reg == NULL);
    testrun(!ov_file_format_register_from_json_from_path(&reg, dir, NULL));
    testrun(reg == NULL);

    for (size_t i = 0; i < items; i++) {

        unlink(files[i]);
        free(files[i]);
    }
    rmdir(dir);
    free(dir);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_file_format_as() {

    TODO(" ... to be implemented");

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_file_format",
            check_parse_extensions,
            test_ov_file_encoding_is_utf8,
            test_ov_file_format_register,
            test_ov_file_format_free_registry,
            test_ov_file_desc_from_path,
            test_ov_file_format_get_desc,
            test_ov_file_format_get,
            test_ov_file_format_get_ext,
            test_ov_file_format_as,
            check_parallel_requests,
            test_ov_file_format_register_values_to_json,
            test_ov_file_format_register_from_json_from_path,
            test_ov_file_format_register_values_from_json);

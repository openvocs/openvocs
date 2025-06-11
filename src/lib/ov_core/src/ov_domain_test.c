/***
        ------------------------------------------------------------------------

        Copyright (c) 2020 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_domain_test.c
        @author         Markus Töpfer

        @date           2020-12-18


        ------------------------------------------------------------------------
*/
#include "ov_domain.c"
#include <ov_test/testrun.h>

#ifndef OV_TEST_RESOURCE_DIR
#error "Must provide -D OV_TEST_RESOURCE_DIR=value while compiling this file."
#endif

#ifndef OV_TEST_CERT
#error "Must provide -D OV_TEST_CERT=value while compiling this file."
#endif

#ifndef OV_TEST_CERT_KEY
#error "Must provide -D OV_TEST_CERT_KEY=value while compiling this file."
#endif

#ifndef OV_TEST_CERT_ONE
#error "Must provide -D OV_TEST_CERT_ONE=value while compiling this file."
#endif

#ifndef OV_TEST_CERT_ONE_KEY
#error "Must provide -D OV_TEST_CERT_ONE_KEY=value while compiling this file."
#endif

#ifndef OV_TEST_CERT_TWO
#error "Must provide -D OV_TEST_CERT_TWO=value while compiling this file."
#endif

#ifndef OV_TEST_CERT_TWO_KEY
#error "Must provide -D OV_TEST_CERT_TWO_KEY=value while compiling this file."
#endif

#include <ov_base/ov_file.h>
#include <sys/stat.h>
#include <sys/types.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_domain_config_from_json() {

    ov_domain_config config = {0};
    ov_domain_config out = {0};
    ov_json_value *cfg = ov_domain_config_to_json(config);

    out = ov_domain_config_from_json(NULL);
    testrun(0 == out.name.start[0]);
    testrun(0 == out.path[0]);
    testrun(0 == out.certificate.cert[0]);
    testrun(0 == out.certificate.key[0]);
    testrun(0 == out.certificate.ca.file[0]);
    testrun(0 == out.certificate.ca.path[0]);

    out = ov_domain_config_from_json(cfg);
    testrun(0 == out.name.start[0]);
    testrun(0 == out.path[0]);
    testrun(0 == out.certificate.cert[0]);
    testrun(0 == out.certificate.key[0]);
    testrun(0 == out.certificate.ca.file[0]);
    testrun(0 == out.certificate.ca.path[0]);
    cfg = ov_json_value_free(cfg);

    config = (ov_domain_config){

        .name = "1",
        .path = "2",
        .certificate.cert = "3",
        .certificate.key = "4",
        .certificate.ca.file = "5",
        .certificate.ca.path = "6"};

    cfg = ov_domain_config_to_json(config);

    out = ov_domain_config_from_json(cfg);
    testrun(0 == strcmp("1", (char *)out.name.start));
    testrun(0 == strcmp("2", out.path));
    testrun(0 == strcmp("3", out.certificate.cert));
    testrun(0 == strcmp("4", out.certificate.key));
    testrun(0 == strcmp("5", out.certificate.ca.file));
    testrun(0 == strcmp("6", out.certificate.ca.path));
    cfg = ov_json_value_free(cfg);

    config = (ov_domain_config){

        .name = "1",
        .path = "2",
        .certificate.cert = "3",
        .certificate.key = "4",
    };

    cfg = ov_domain_config_to_json(config);

    out = ov_domain_config_from_json(cfg);
    testrun(0 == strcmp("1", (char *)out.name.start));
    testrun(0 == strcmp("2", out.path));
    testrun(0 == strcmp("3", out.certificate.cert));
    testrun(0 == strcmp("4", out.certificate.key));
    testrun(0 == out.certificate.ca.file[0]);
    testrun(0 == out.certificate.ca.path[0]);
    cfg = ov_json_value_free(cfg);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_domain_config_to_json() {

    ov_domain_config config = {0};
    ov_json_value *out = ov_domain_config_to_json(config);
    testrun(out);
    testrun(ov_json_get(out, "/" OV_KEY_PATH));
    testrun(ov_json_get(out, "/" OV_KEY_NAME));
    testrun(ov_json_get(out, "/" OV_KEY_CERTIFICATE));
    testrun(ov_json_get(out, "/" OV_KEY_CERTIFICATE "/" OV_KEY_KEY));
    testrun(ov_json_get(out, "/" OV_KEY_CERTIFICATE "/" OV_KEY_FILE));
    testrun(ov_json_get(out, "/" OV_KEY_CERTIFICATE "/" OV_KEY_AUTHORITY));
    testrun(ov_json_get(
        out, "/" OV_KEY_CERTIFICATE "/" OV_KEY_AUTHORITY "/" OV_KEY_FILE));
    testrun(ov_json_get(
        out, "/" OV_KEY_CERTIFICATE "/" OV_KEY_AUTHORITY "/" OV_KEY_PATH));

    testrun(ov_json_is_null(ov_json_get(out, "/" OV_KEY_PATH)));
    testrun(ov_json_is_null(ov_json_get(out, "/" OV_KEY_NAME)));
    testrun(ov_json_is_null(
        ov_json_get(out, "/" OV_KEY_CERTIFICATE "/" OV_KEY_KEY)));
    testrun(ov_json_is_null(
        ov_json_get(out, "/" OV_KEY_CERTIFICATE "/" OV_KEY_FILE)));
    testrun(ov_json_is_null(ov_json_get(
        out, "/" OV_KEY_CERTIFICATE "/" OV_KEY_AUTHORITY "/" OV_KEY_FILE)));
    testrun(ov_json_is_null(ov_json_get(
        out, "/" OV_KEY_CERTIFICATE "/" OV_KEY_AUTHORITY "/" OV_KEY_PATH)));

    out = ov_json_value_free(out);

    config = (ov_domain_config){

        .name.start = "1",
        .path = "2",
        .certificate.cert = "3",
        .certificate.key = "4",
        .certificate.ca.file = "5",
        .certificate.ca.path = "6"};

    out = ov_domain_config_to_json(config);
    testrun(out);
    testrun(ov_json_get(out, "/" OV_KEY_PATH));
    testrun(ov_json_get(out, "/" OV_KEY_NAME));
    testrun(ov_json_get(out, "/" OV_KEY_CERTIFICATE));
    testrun(ov_json_get(out, "/" OV_KEY_CERTIFICATE "/" OV_KEY_KEY));
    testrun(ov_json_get(out, "/" OV_KEY_CERTIFICATE "/" OV_KEY_FILE));
    testrun(ov_json_get(out, "/" OV_KEY_CERTIFICATE "/" OV_KEY_AUTHORITY));
    testrun(ov_json_get(
        out, "/" OV_KEY_CERTIFICATE "/" OV_KEY_AUTHORITY "/" OV_KEY_FILE));
    testrun(ov_json_get(
        out, "/" OV_KEY_CERTIFICATE "/" OV_KEY_AUTHORITY "/" OV_KEY_PATH));

    testrun(0 ==
            strcmp("1", ov_json_string_get(ov_json_get(out, "/" OV_KEY_NAME))));
    testrun(0 ==
            strcmp("2", ov_json_string_get(ov_json_get(out, "/" OV_KEY_PATH))));
    testrun(0 == strcmp("3",
                        ov_json_string_get(ov_json_get(
                            out, "/" OV_KEY_CERTIFICATE "/" OV_KEY_FILE))));
    testrun(0 == strcmp("4",
                        ov_json_string_get(ov_json_get(
                            out, "/" OV_KEY_CERTIFICATE "/" OV_KEY_KEY))));
    testrun(0 == strcmp("5",
                        ov_json_string_get(ov_json_get(out,
                                                       "/" OV_KEY_CERTIFICATE
                                                       "/" OV_KEY_AUTHORITY
                                                       "/" OV_KEY_FILE))));
    testrun(0 == strcmp("6",
                        ov_json_string_get(ov_json_get(out,
                                                       "/" OV_KEY_CERTIFICATE
                                                       "/" OV_KEY_AUTHORITY
                                                       "/" OV_KEY_PATH))));
    out = ov_json_value_free(out);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_domain_init() {

    ov_domain check = (ov_domain){.magic_byte = OV_DOMAIN_MAGIC_BYTE};

    ov_domain domain = {0};
    ov_domain_config config = (ov_domain_config){0};

    testrun(!ov_domain_init(NULL));

    testrun(ov_domain_init(&domain));
    testrun(NULL == domain.websocket.uri);

    testrun(0 == memcmp(&domain.config, &config, sizeof(ov_domain_config)));

    testrun(strcat((char *)domain.config.name.start, "test"));
    testrun(strcat(domain.config.path, "sadadadad"));
    testrun(strcat(domain.config.certificate.cert, "certificate"));
    testrun(strcat(domain.config.certificate.key, "sadadafadasfasfsaf"));
    testrun(strcat(domain.config.certificate.ca.file, "sdsdsdsdsdds"));
    testrun(strcat(domain.config.certificate.ca.path, "fonafnafpnpanfpasnfpn"));

    domain.websocket.uri = ov_dict_free(domain.websocket.uri);

    testrun(0 != memcmp(&check, &domain, sizeof(ov_domain)));
    testrun(0 != memcmp(&domain.config, &config, sizeof(ov_domain_config)));

    testrun(ov_domain_init(&domain));
    testrun(NULL == domain.websocket.uri);

    testrun(0 == memcmp(&domain.config, &config, sizeof(ov_domain_config)));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_domain_config_verify() {

    testrun(ov_dir_access_to_path(OV_TEST_RESOURCE_DIR));
    testrun(0 != ov_file_read_check("/filedoesnotexist"));

    ov_domain_config config = {0};
    testrun(!ov_domain_config_verify(NULL));
    testrun(!ov_domain_config_verify(&config));

    // minimal config
    config = (ov_domain_config){.name.start = "test",
                                .path = OV_TEST_RESOURCE_DIR,
                                .certificate.cert = OV_TEST_CERT,
                                .certificate.key = OV_TEST_CERT_KEY};
    testrun(ov_domain_config_verify(&config));

    // no name
    config = (ov_domain_config){.path = OV_TEST_RESOURCE_DIR,
                                .certificate.cert = OV_TEST_CERT,
                                .certificate.key = OV_TEST_CERT_KEY};
    testrun(!ov_domain_config_verify(&config));

    // invalid path
    config = (ov_domain_config){.name.start = "test",
                                .path = "/filedoesnotexist",
                                .certificate.cert = OV_TEST_CERT,
                                .certificate.key = OV_TEST_CERT_KEY};
    testrun(!ov_domain_config_verify(&config));

    // invalid cert
    config = (ov_domain_config){.name.start = "test",
                                .path = OV_TEST_RESOURCE_DIR,
                                .certificate.cert = "/filedoesnotexist",
                                .certificate.key = OV_TEST_CERT_KEY};
    testrun(!ov_domain_config_verify(&config));

    // invalid key
    config = (ov_domain_config){.name.start = "test",
                                .path = OV_TEST_RESOURCE_DIR,
                                .certificate.cert = OV_TEST_CERT,
                                .certificate.key = "/filedoesnotexist"};
    testrun(!ov_domain_config_verify(&config));

    // invalid ca file
    config = (ov_domain_config){.name.start = "test",
                                .path = OV_TEST_RESOURCE_DIR,
                                .certificate.cert = OV_TEST_CERT,
                                .certificate.key = OV_TEST_CERT_KEY,
                                .certificate.ca.file = "/filedoesnotexist"};
    testrun(!ov_domain_config_verify(&config));

    // valid ca file
    config = (ov_domain_config){.name.start = "test",
                                .path = OV_TEST_RESOURCE_DIR,
                                .certificate.cert = OV_TEST_CERT,
                                .certificate.key = OV_TEST_CERT_KEY,
                                .certificate.ca.file = OV_TEST_CERT};
    testrun(ov_domain_config_verify(&config));

    // invalid ca path
    config = (ov_domain_config){.name.start = "test",
                                .path = OV_TEST_RESOURCE_DIR,
                                .certificate.cert = OV_TEST_CERT,
                                .certificate.key = OV_TEST_CERT_KEY,
                                .certificate.ca.file = OV_TEST_CERT,
                                .certificate.ca.path = "/filedoesnotexist"};
    testrun(!ov_domain_config_verify(&config));

    // valid ca path
    config = (ov_domain_config){.name.start = "test",
                                .path = OV_TEST_RESOURCE_DIR,
                                .certificate.cert = OV_TEST_CERT,
                                .certificate.key = OV_TEST_CERT_KEY,
                                .certificate.ca.file = OV_TEST_CERT,
                                .certificate.ca.path = OV_TEST_RESOURCE_DIR};
    testrun(ov_domain_config_verify(&config));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_domain_load() {

    const char *file1 = OV_TEST_RESOURCE_DIR "domain1";
    const char *file2 = OV_TEST_RESOURCE_DIR "domain2";
    const char *file3 = OV_TEST_RESOURCE_DIR "domain3";
    const char *file4 = OV_TEST_RESOURCE_DIR "domain_config_invalid";

    unlink(file1);
    unlink(file2);
    unlink(file3);
    unlink(file4);

    ov_json_value *conf = ov_json_object();
    ov_json_value *cert = ov_json_object();
    testrun(ov_json_object_set(conf, OV_KEY_CERTIFICATE, cert));

    ov_json_value *val = NULL;
    val = ov_json_string("domain1");
    testrun(ov_json_object_set(conf, OV_KEY_NAME, val));
    val = ov_json_string(OV_TEST_RESOURCE_DIR);
    testrun(ov_json_object_set(conf, OV_KEY_PATH, val));
    val = ov_json_string(OV_TEST_CERT);
    testrun(ov_json_object_set(cert, OV_KEY_FILE, val));
    val = ov_json_string(OV_TEST_CERT_KEY);
    testrun(ov_json_object_set(cert, OV_KEY_KEY, val));
    char *str = ov_json_value_to_string(conf);
    fprintf(stdout, "%s\n", str);
    str = ov_data_pointer_free(str);

    testrun(ov_json_write_file(file1, conf));

    val = ov_json_string("domain2");
    testrun(ov_json_object_set(conf, OV_KEY_NAME, val));
    testrun(ov_json_write_file(file2, conf));

    val = ov_json_string("domain3");
    testrun(ov_json_object_set(conf, OV_KEY_NAME, val));
    testrun(ov_json_write_file(file3, conf));

    testrun(ov_dir_access_to_path(OV_TEST_RESOURCE_DIR));
    testrun(0 == ov_file_read_check(file1));
    testrun(0 == ov_file_read_check(file2));
    testrun(0 == ov_file_read_check(file3));
    testrun(0 == ov_file_read_check(OV_TEST_RESOURCE_DIR "/test_file"));
    // file 4 not written yet
    testrun(0 != ov_file_read_check(file4));

    size_t size = 0;
    ov_domain *array = NULL;

    testrun(!ov_domain_load(NULL, NULL, NULL));
    testrun(!ov_domain_load(NULL, &size, &array));
    testrun(!ov_domain_load(OV_TEST_RESOURCE_DIR, NULL, &array));
    testrun(!ov_domain_load(OV_TEST_RESOURCE_DIR, &size, NULL));

    testrun(0 == size);
    testrun(NULL == array);
    testrun(ov_domain_load(OV_TEST_RESOURCE_DIR, &size, &array));

    testrun(3 == size);
    testrun(NULL != array);

    // check domain configs
    for (size_t i = 0; i < size; i++) {

        // order may be different due to keying mechanism of JSON
        if (0 != strcmp((char *)array[i].config.name.start, "domain1"))
            if (0 != strcmp((char *)array[i].config.name.start, "domain2"))
                testrun(0 ==
                        strcmp((char *)array[i].config.name.start, "domain3"));

        testrun(0 == strcmp(array[i].config.path, OV_TEST_RESOURCE_DIR));
        testrun(0 == strcmp(array[i].config.certificate.cert, OV_TEST_CERT));
        testrun(0 == strcmp(array[i].config.certificate.key, OV_TEST_CERT_KEY));
        testrun(0 == array[i].config.certificate.ca.file[0]);
        testrun(0 == array[i].config.certificate.ca.path[0]);
    }

    // check with array pointer
    testrun(!ov_domain_load(OV_TEST_RESOURCE_DIR, &size, &array));
    array = ov_domain_array_free(size, array);
    size = 0;

    // check with authority
    ov_json_value *auth = ov_json_object();
    testrun(ov_json_object_set(cert, OV_KEY_AUTHORITY, auth));
    val = ov_json_string(OV_TEST_CERT);
    testrun(ov_json_object_set(auth, OV_KEY_FILE, val));
    testrun(ov_json_write_file(file3, conf));

    testrun(ov_domain_load(OV_TEST_RESOURCE_DIR, &size, &array));

    testrun(3 == size);
    testrun(NULL != array);

    // check domain configs
    for (size_t i = 0; i < size; i++) {

        // order may be different due to keying mechanism of JSON
        if (0 != strcmp((char *)array[i].config.name.start, "domain1"))
            if (0 != strcmp((char *)array[i].config.name.start, "domain2"))
                testrun(0 ==
                        strcmp((char *)array[i].config.name.start, "domain3"));

        testrun(0 == strcmp(array[i].config.path, OV_TEST_RESOURCE_DIR));
        testrun(0 == strcmp(array[i].config.certificate.cert, OV_TEST_CERT));
        testrun(0 == strcmp(array[i].config.certificate.key, OV_TEST_CERT_KEY));

        if (0 != strcmp((char *)array[i].config.name.start, "domain3")) {
            testrun(0 == array[i].config.certificate.ca.file[0]);
            testrun(0 == array[i].config.certificate.ca.path[0]);
        } else {
            testrun(0 ==
                    strcmp(array[i].config.certificate.ca.file, OV_TEST_CERT));
            testrun(0 == array[i].config.certificate.ca.path[0]);
        }
    }

    array = ov_domain_array_free(size, array);
    size = 0;

    // check with invalid config
    val = ov_json_string("/filedoesnotexist");
    testrun(ov_json_object_set(conf, OV_KEY_PATH, val));
    testrun(ov_json_write_file(file4, conf));

    testrun(ov_dir_access_to_path(OV_TEST_RESOURCE_DIR));
    testrun(0 == ov_file_read_check(file1));
    testrun(0 == ov_file_read_check(file2));
    testrun(0 == ov_file_read_check(file3));
    testrun(0 == ov_file_read_check(OV_TEST_RESOURCE_DIR "/test_file"));
    testrun(0 == ov_file_read_check(file4));
    testrun(!ov_domain_load(OV_TEST_RESOURCE_DIR, &size, &array));
    testrun(0 == size);
    testrun(NULL == array);

    // reverify with valid path in file4
    val = ov_json_string(OV_TEST_RESOURCE_DIR);
    testrun(ov_json_object_set(conf, OV_KEY_PATH, val));
    testrun(ov_json_write_file(file4, conf));
    testrun(ov_domain_load(OV_TEST_RESOURCE_DIR, &size, &array));
    testrun(4 == size);
    testrun(NULL != array);
    array = ov_domain_array_free(size, array);

    // test without any JSON config
    unlink(file1);
    unlink(file2);
    unlink(file3);
    unlink(file4);

    testrun(ov_dir_access_to_path(OV_TEST_RESOURCE_DIR));
    testrun(0 != ov_file_read_check(file1));
    testrun(0 != ov_file_read_check(file2));
    testrun(0 != ov_file_read_check(file3));
    testrun(0 == ov_file_read_check(OV_TEST_RESOURCE_DIR "/test_file"));
    testrun(0 != ov_file_read_check(file4));

    size = 150;
    testrun(!ov_domain_load(OV_TEST_RESOURCE_DIR, &size, &array));
    testrun(0 == size);
    testrun(NULL == array);

    size = 0;
    testrun(!ov_domain_load(OV_TEST_RESOURCE_DIR, &size, &array));
    testrun(0 == size);
    testrun(NULL == array);

    conf = ov_json_value_free(conf);
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_domain_array_clean() {

    ov_domain domain = {.magic_byte = OV_DOMAIN_MAGIC_BYTE};
    ov_domain array[10] = {0};

    for (size_t i = 0; i < 10; i++) {
        testrun(ov_domain_init(&array[i]));
        testrun(strcat((char *)&array[i].config.name.start, "test"));
        testrun(strcat(array[i].config.path, "sadadadad"));
        testrun(strcat(array[i].config.certificate.cert, "certificate"));
        testrun(strcat(array[i].config.certificate.key, "sadadafadasfasfsaf"));
        testrun(strcat(array[i].config.certificate.ca.file, "sdsdsdsdsdds"));
        testrun(strcat(
            array[i].config.certificate.ca.path, "fonafnafpnpanfpasnfpn"));
    }

    ov_domain *arr = array;

    testrun(!ov_domain_array_clean(0, NULL));
    testrun(ov_domain_array_clean(0, arr));
    testrun(ov_domain_array_clean(5, arr));

    for (size_t i = 0; i < 10; i++) {

        if (i < 5) {
            testrun(0 == memcmp(&array[i], &domain, sizeof(ov_domain)));
        } else {
            testrun(0 != memcmp(&array[i], &domain, sizeof(ov_domain)));
        }
    }

    testrun(ov_domain_array_clean(10, arr));

    for (size_t i = 0; i < 10; i++) {

        testrun(0 == memcmp(&array[i], &domain, sizeof(ov_domain)));
    }

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_domain_array_free() {

    ov_domain *array = calloc(10, sizeof(ov_domain));

    for (size_t i = 0; i < 10; i++) {
        testrun(ov_domain_init(&array[i]));
        testrun(strcat((char *)array[i].config.name.start, "test"));
        testrun(strcat(array[i].config.path, "sadadadad"));
        testrun(strcat(array[i].config.certificate.cert, "certificate"));
        testrun(strcat(array[i].config.certificate.key, "sadadafadasfasfsaf"));
        testrun(strcat(array[i].config.certificate.ca.file, "sdsdsdsdsdds"));
        testrun(strcat(
            array[i].config.certificate.ca.path, "fonafnafpnpanfpasnfpn"));
    }

    array = ov_domain_array_free(10, array);

    testrun_log("... check valgrind clean run");

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_domain_deinit_tls_context() {

    ov_domain domain = {0};

    domain.context.tls = SSL_CTX_new(TLS_server_method());
    testrun(domain.context.tls);
    ov_domain_deinit_tls_context(NULL);

    testrun(domain.context.tls);
    ov_domain_deinit_tls_context(&domain);
    testrun(NULL == domain.context.tls);

    ov_domain_deinit_tls_context(&domain);
    testrun(NULL == domain.context.tls);

    return testrun_log_success();
}

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CLUSTER                                                    #CLUSTER
 *
 *      ------------------------------------------------------------------------
 */

int all_tests() {

    testrun_init();

    testrun_test(test_ov_domain_init);
    testrun_test(test_ov_domain_config_verify);

    testrun_test(test_ov_domain_load);

    testrun_test(test_ov_domain_array_clean);
    testrun_test(test_ov_domain_array_free);

    testrun_test(test_ov_domain_deinit_tls_context);

    testrun_test(test_ov_domain_config_from_json);
    testrun_test(test_ov_domain_config_to_json);

    return testrun_counter;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST EXECUTION                                                  #EXEC
 *
 *      ------------------------------------------------------------------------
 */

testrun_run(all_tests);

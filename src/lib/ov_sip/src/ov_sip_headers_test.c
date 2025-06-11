/***
        ------------------------------------------------------------------------

        Copyright (c) 2022 German Aerospace Center DLR e.V. (GSOC)

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

#include "ov_sip_headers.c"
#include <ov_base/ov_registered_cache.h>
#include <ov_test/ov_test.h>

/*----------------------------------------------------------------------------*/

static int test_ov_sip_headers_auth() {

    char const *expected =
        "Digest realm=\"asterisk\", "
        "nonce=\"1664d00a\", algorithm=MD5, username=\"7247\", "
        "uri=\"sip:10.61.48.200\", "
        "response=\"6bf6b437c3337be69fb74a0f72f4fca1\"";

    testrun(0 == ov_sip_headers_auth(0, 0, 0, 0, 0, 0));
    testrun(0 == ov_sip_headers_auth("sip:a.b", 0, 0, 0, 0, 0));
    testrun(0 == ov_sip_headers_auth(0, "7", 0, 0, 0, 0));
    testrun(0 == ov_sip_headers_auth("sip:a.b", "7", 0, 0, 0, 0));
    testrun(0 == ov_sip_headers_auth(0, 0, "abc", 0, 0, 0));
    testrun(0 == ov_sip_headers_auth("sip:a.b", 0, "abc", 0, 0, 0));
    testrun(0 == ov_sip_headers_auth(0, "7", "abc", 0, 0, 0));
    testrun(0 == ov_sip_headers_auth("sip:a.b", "7", "abc", 0, 0, 0));
    testrun(0 == ov_sip_headers_auth(0, 0, 0, "realm", 0, 0));
    testrun(0 == ov_sip_headers_auth("sip:a.b", 0, 0, "realm", 0, 0));
    testrun(0 == ov_sip_headers_auth(0, "7", 0, "realm", 0, 0));
    testrun(0 == ov_sip_headers_auth("sip:a.b", "7", 0, "realm", 0, 0));
    testrun(0 == ov_sip_headers_auth(0, 0, "abc", "realm", 0, 0));
    testrun(0 == ov_sip_headers_auth("sip:a.b", 0, "abc", "realm", 0, 0));
    testrun(0 == ov_sip_headers_auth(0, "7", "abc", "realm", 0, 0));
    testrun(0 == ov_sip_headers_auth("sip:a.b", "7", "abc", "realm", 0, 0));

    testrun(0 == ov_sip_headers_auth(0, 0, 0, 0, "REGISTER", 0));
    testrun(0 == ov_sip_headers_auth("sip:a.b", 0, 0, 0, "REGISTER", 0));
    testrun(0 == ov_sip_headers_auth(0, "7", 0, 0, "REGISTER", 0));
    testrun(0 == ov_sip_headers_auth("sip:a.b", "7", 0, 0, "REGISTER", 0));
    testrun(0 == ov_sip_headers_auth(0, 0, "abc", 0, "REGISTER", 0));
    testrun(0 == ov_sip_headers_auth("sip:a.b", 0, "abc", 0, "REGISTER", 0));
    testrun(0 == ov_sip_headers_auth(0, "7", "abc", 0, "REGISTER", 0));
    testrun(0 == ov_sip_headers_auth("sip:a.b", "7", "abc", 0, "REGISTER", 0));
    testrun(0 == ov_sip_headers_auth(0, 0, 0, "realm", "REGISTER", 0));
    testrun(0 == ov_sip_headers_auth("sip:a.b", 0, 0, "realm", "REGISTER", 0));
    testrun(0 == ov_sip_headers_auth(0, "7", 0, "realm", "REGISTER", 0));
    testrun(0 ==
            ov_sip_headers_auth("sip:a.b", "7", 0, "realm", "REGISTER", 0));
    testrun(0 == ov_sip_headers_auth(0, 0, "abc", "realm", "REGISTER", 0));
    testrun(0 ==
            ov_sip_headers_auth("sip:a.b", 0, "abc", "realm", "REGISTER", 0));
    testrun(0 == ov_sip_headers_auth(0, "7", "abc", "realm", "REGISTER", 0));
    testrun(0 ==
            ov_sip_headers_auth("sip:a.b", "7", "abc", "realm", "REGISTER", 0));

    testrun(0 == ov_sip_headers_auth(0, 0, 0, 0, 0, "nonce"));
    testrun(0 == ov_sip_headers_auth("sip:a.b", 0, 0, 0, 0, "nonce"));
    testrun(0 == ov_sip_headers_auth(0, "7", 0, 0, 0, "nonce"));
    testrun(0 == ov_sip_headers_auth("sip:a.b", "7", 0, 0, 0, "nonce"));
    testrun(0 == ov_sip_headers_auth(0, 0, "abc", 0, 0, "nonce"));
    testrun(0 == ov_sip_headers_auth("sip:a.b", 0, "abc", 0, 0, "nonce"));
    testrun(0 == ov_sip_headers_auth(0, "7", "abc", 0, 0, "nonce"));
    testrun(0 == ov_sip_headers_auth("sip:a.b", "7", "abc", 0, 0, "nonce"));
    testrun(0 == ov_sip_headers_auth(0, 0, 0, "realm", 0, "nonce"));
    testrun(0 == ov_sip_headers_auth("sip:a.b", 0, 0, "realm", 0, "nonce"));
    testrun(0 == ov_sip_headers_auth(0, "7", 0, "realm", 0, "nonce"));
    testrun(0 == ov_sip_headers_auth("sip:a.b", "7", 0, "realm", 0, "nonce"));
    testrun(0 == ov_sip_headers_auth(0, 0, "abc", "realm", 0, "nonce"));
    testrun(0 == ov_sip_headers_auth("sip:a.b", 0, "abc", "realm", 0, "nonce"));
    testrun(0 ==
            ov_sip_headers_auth("sip:a.b", "7", "abc", "realm", 0, "nonce"));

    testrun(0 == ov_sip_headers_auth(0, 0, 0, 0, "REGISTER", "nonce"));
    testrun(0 == ov_sip_headers_auth("sip:a.b", 0, 0, 0, "REGISTER", "nonce"));
    testrun(0 == ov_sip_headers_auth(0, "7", 0, 0, "REGISTER", "nonce"));
    testrun(0 ==
            ov_sip_headers_auth("sip:a.b", "7", 0, 0, "REGISTER", "nonce"));
    testrun(0 == ov_sip_headers_auth(0, 0, "abc", 0, "REGISTER", "nonce"));
    testrun(0 ==
            ov_sip_headers_auth("sip:a.b", 0, "abc", 0, "REGISTER", "nonce"));
    testrun(0 == ov_sip_headers_auth(0, "7", "abc", 0, "REGISTER", "nonce"));
    testrun(0 ==
            ov_sip_headers_auth("sip:a.b", "7", "abc", 0, "REGISTER", "nonce"));
    testrun(0 == ov_sip_headers_auth(0, 0, 0, "realm", "REGISTER", "nonce"));
    testrun(0 ==
            ov_sip_headers_auth("sip:a.b", 0, 0, "realm", "REGISTER", "nonce"));
    testrun(0 == ov_sip_headers_auth(0, "7", 0, "realm", "REGISTER", "nonce"));
    testrun(0 == ov_sip_headers_auth(
                     "sip:a.b", "7", 0, "realm", "REGISTER", "nonce"));
    testrun(0 ==
            ov_sip_headers_auth(0, 0, "abc", "realm", "REGISTER", "nonce"));
    testrun(0 == ov_sip_headers_auth(
                     "sip:a.b", 0, "abc", "realm", "REGISTER", "nonce"));
    testrun(0 ==
            ov_sip_headers_auth(0, "7", "abc", "realm", "REGISTER", "nonce"));

    ov_buffer *header = ov_sip_headers_auth("sip:10.61.48.200", // uri
                                            "7247",             // user
                                            "7427abc",          // password
                                            "asterisk",         // realm
                                            "REGISTER",         // sip-method
                                            "1664d00a");        // nonce

    testrun(0 != header);
    testrun(strlen(expected) + 1 == header->length);
    testrun(0 == memcmp(header->start, expected, header->length));

    header = ov_buffer_free(header);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int ov_sip_headers_get_user_and_tag_test() {

    testrun(ov_sip_headers_get_user_and_tag(0, 0, 0));

    testrun(ov_sip_headers_get_user_and_tag("anton", 0, 0));

    ov_buffer *user = 0;

    testrun(ov_sip_headers_get_user_and_tag("anton", &user, 0));
    testrun(ov_buffer_equals(user, "anton"));

    /* No need to release `user` if we feed it back in to ov_su_user_from */

    testrun(ov_sip_headers_get_user_and_tag("anton@hofreiter", &user, 0));
    testrun(ov_buffer_equals(user, "anton@hofreiter"));

    /* But we can delete `user` */
    user = ov_buffer_free(user);

    testrun(ov_sip_headers_get_user_and_tag(
        "anton@hofreiter;tag=wurzelsepp", &user, 0));
    testrun(ov_buffer_equals(user, "anton@hofreiter"));
    user = ov_buffer_free(user);

    ov_buffer *tag = 0;
    testrun(ov_sip_headers_get_user_and_tag(
        "anton@hofreiter;tag=wurzelsepp", &user, &tag));
    testrun(ov_buffer_equals(user, "anton@hofreiter"));
    testrun(ov_buffer_equals(tag, "wurzelsepp"));

    /* No need to release `tag` if we feed it back in to ov_su_user_from */

    testrun(ov_sip_headers_get_user_and_tag(
        "<anton@walderado>;tag=waldschrat", &user, &tag));
    testrun(ov_buffer_equals(user, "anton@walderado"));
    testrun(ov_buffer_equals(tag, "waldschrat"));

    /* but again, we can free `tag` */
    tag = ov_buffer_free(tag);

    testrun(ov_sip_headers_get_user_and_tag(
        "<bruno@walderado;transport=TCP>;tag=pilzkopf", &user, &tag));
    testrun(ov_buffer_equals(user, "bruno@walderado"));
    testrun(ov_buffer_equals(tag, "pilzkopf"));

    testrun(ov_sip_headers_get_user_and_tag(
        "<zottel_aber_gut:anton@hofreiter>", &user, 0));
    testrun(ov_buffer_equals(user, "anton@hofreiter"));

    testrun(ov_sip_headers_get_user_and_tag(
        "<anton@hofreiter;transport=a:b>;tag=ich_ess_blumen", &user, &tag));

    testrun(ov_buffer_equals(user, "anton@hofreiter"));
    testrun(ov_buffer_equals(tag, "ich_ess_blumen"));
    testrun(0 > strcmp((char const *)user->start, "anton@hofreiter>"));

    testrun(ov_sip_headers_get_user_and_tag(
        "<zottel_aber_gut:anton@hofreiter>;tag=indianertot", &user, 0));
    testrun(ov_buffer_equals(user, "anton@hofreiter"));
    testrun(0 > strcmp((char const *)user->start, "anton@hofreiter>"));

    testrun(ov_sip_headers_get_user_and_tag(
        "<zottel_aber_gut:anton@hofreiter>;tag=indianertot", &user, &tag));

    testrun(ov_buffer_equals(user, "anton@hofreiter"));
    testrun(ov_buffer_equals(tag, "indianertot"));
    testrun(0 > strcmp((char const *)user->start, "anton@hofreiter>"));

    testrun(ov_sip_headers_get_user_and_tag(
        "\"anton\"  <zottel_aber_gut:anton@hofreiter>;tag=indianertot",
        &user,
        &tag));

    testrun(ov_buffer_equals(user, "anton@hofreiter"));
    testrun(ov_buffer_equals(tag, "indianertot"));
    testrun(0 > strcmp((char const *)user->start, "anton@hofreiter>"));

    testrun(ov_sip_headers_get_user_and_tag(
        "\"anton\"  <anton@hofreiter>;tag=indianertot", &user, &tag));

    testrun(ov_buffer_equals(user, "anton@hofreiter"));
    testrun(ov_buffer_equals(tag, "indianertot"));
    testrun(0 > strcmp((char const *)user->start, "anton@hofreiter>"));

    user = ov_buffer_free(user);
    tag = ov_buffer_free(tag);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int tear_down() {

    ov_registered_cache_free_all();
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_sip_app",
            test_ov_sip_headers_auth,
            ov_sip_headers_get_user_and_tag_test,
            tear_down);

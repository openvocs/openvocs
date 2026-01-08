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
        @file           ov_sdp_attribute_test.c
        @author         Markus Töpfer

        @date           2019-12-07

        @ingroup        ov_sdp_attribute

        @brief          Unit tests of


        ------------------------------------------------------------------------
*/

#include "ov_sdp_attribute.c"

#include "../../include/ov_buffer.h"
#include "../../include/ov_sdp.h"
#include <ov_test/testrun.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_sdp_attribute_is_set() {

  ov_sdp_list attr1 = (ov_sdp_list){.key = "1"};
  ov_sdp_list attr2 = (ov_sdp_list){.key = "2"};
  ov_sdp_list attr3 = (ov_sdp_list){.key = "3"};

  ov_sdp_list *head = NULL;
  testrun(ov_node_push((void **)&head, &attr1));
  testrun(ov_node_push((void **)&head, &attr2));
  testrun(ov_node_push((void **)&head, &attr3));

  testrun(3 == ov_node_count(head));

  testrun(!ov_sdp_attribute_is_set(NULL, NULL));
  testrun(!ov_sdp_attribute_is_set(head, NULL));
  testrun(!ov_sdp_attribute_is_set(NULL, "1"));
  testrun(ov_sdp_attribute_is_set(head, "1"));
  testrun(ov_sdp_attribute_is_set(head, "2"));
  testrun(ov_sdp_attribute_is_set(head, "3"));
  testrun(!ov_sdp_attribute_is_set(head, "4"));
  testrun(!ov_sdp_attribute_is_set(head, "5"));
  testrun(!ov_sdp_attribute_is_set(head, "6"));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_attribute_get() {

  const char *v1 = "one";
  const char *v2 = "two";
  const char *v3 = "three";

  ov_sdp_list attr1 = (ov_sdp_list){.key = "1", .value = v1};
  ov_sdp_list attr2 = (ov_sdp_list){.key = "2", .value = v2};
  ov_sdp_list attr3 = (ov_sdp_list){.key = "3", .value = v3};

  ov_sdp_list *head = NULL;
  testrun(ov_node_push((void **)&head, &attr1));
  testrun(ov_node_push((void **)&head, &attr2));
  testrun(ov_node_push((void **)&head, &attr3));

  testrun(3 == ov_node_count(head));

  testrun(!ov_sdp_attribute_get(NULL, NULL));
  testrun(!ov_sdp_attribute_get(head, NULL));
  testrun(!ov_sdp_attribute_get(NULL, "1"));
  testrun(v1 == ov_sdp_attribute_get(head, "1"));
  testrun(v2 == ov_sdp_attribute_get(head, "2"));
  testrun(v3 == ov_sdp_attribute_get(head, "3"));
  testrun(!ov_sdp_attribute_get(head, "4"));
  testrun(!ov_sdp_attribute_get(head, "5"));
  testrun(!ov_sdp_attribute_get(head, "6"));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_attribute_iterate() {

  const char *v1 = "one";
  const char *v2 = "two";
  const char *v3 = "three";
  const char *v4 = "four";
  const char *v5 = "five";
  const char *v6 = "six";

  ov_sdp_list attr1 = (ov_sdp_list){.key = "1", .value = v1};
  ov_sdp_list attr2 = (ov_sdp_list){.key = "2", .value = v2};
  ov_sdp_list attr3 = (ov_sdp_list){.key = "1", .value = v3};
  ov_sdp_list attr4 = (ov_sdp_list){.key = "x", .value = v4};
  ov_sdp_list attr5 = (ov_sdp_list){.key = "x", .value = v5};
  ov_sdp_list attr6 = (ov_sdp_list){.key = "x", .value = v6};

  ov_sdp_list *head = NULL;
  ov_sdp_list *iter = head;
  testrun(ov_node_push((void **)&head, &attr1));
  testrun(ov_node_push((void **)&head, &attr2));
  testrun(ov_node_push((void **)&head, &attr3));
  testrun(ov_node_push((void **)&head, &attr4));
  testrun(ov_node_push((void **)&head, &attr5));
  testrun(ov_node_push((void **)&head, &attr6));

  testrun(6 == ov_node_count(head));
  iter = head;

  const char *out = NULL;

  testrun(!ov_sdp_attributes_iterate(NULL, NULL, NULL));
  testrun(!ov_sdp_attributes_iterate(NULL, "1", &out));
  testrun(!ov_sdp_attributes_iterate(&iter, NULL, &out));
  testrun(!ov_sdp_attributes_iterate(&iter, "1", NULL));

  testrun(iter == &attr1);
  testrun(out == NULL);
  testrun(ov_sdp_attributes_iterate(&iter, "1", &out));
  testrun(iter == &attr2);
  testrun(out == v1);
  testrun(ov_sdp_attributes_iterate(&iter, "1", &out));
  testrun(iter == &attr4);
  testrun(out == v3);
  testrun(!ov_sdp_attributes_iterate(&iter, "1", &out));
  testrun(iter == &attr4);
  testrun(out == NULL);

  iter = head;
  out = NULL;
  testrun(ov_sdp_attributes_iterate(&iter, "2", &out));
  testrun(iter == &attr3);
  testrun(out == v2);

  iter = head;
  out = NULL;
  testrun(ov_sdp_attributes_iterate(&iter, "x", &out));
  testrun(iter == &attr5);
  testrun(out == v4);
  testrun(ov_sdp_attributes_iterate(&iter, "x", &out));
  testrun(iter == &attr6);
  testrun(out == v5);
  testrun(ov_sdp_attributes_iterate(&iter, "x", &out));
  testrun(iter == NULL);
  testrun(out == v6);
  testrun(!ov_sdp_attributes_iterate(&iter, "x", &out));
  testrun(iter == NULL);
  testrun(out == NULL);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_is_recvonly() {

  ov_sdp_description desc;
  memset(&desc, 0, sizeof(desc));

  ov_sdp_list attr1 = (ov_sdp_list){.key = "1"};
  ov_sdp_list attr2 = (ov_sdp_list){.key = "recvonly"};
  ov_sdp_list attr3 = (ov_sdp_list){.key = "3"};

  testrun(!ov_sdp_is_recvonly(NULL));
  testrun(!ov_sdp_is_recvonly(&desc));

  testrun(ov_node_push((void **)&desc.attributes, &attr1));
  testrun(ov_node_push((void **)&desc.attributes, &attr2));
  testrun(ov_node_push((void **)&desc.attributes, &attr3));

  testrun(3 == ov_node_count(desc.attributes));
  testrun(ov_sdp_is_recvonly(&desc));
  attr2.key = "2";
  testrun(!ov_sdp_is_recvonly(&desc));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_is_sendonly() {

  ov_sdp_description desc;
  memset(&desc, 0, sizeof(desc));

  ov_sdp_list attr1 = (ov_sdp_list){.key = "1"};
  ov_sdp_list attr2 = (ov_sdp_list){.key = "sendonly"};
  ov_sdp_list attr3 = (ov_sdp_list){.key = "3"};

  testrun(!ov_sdp_is_sendonly(NULL));
  testrun(!ov_sdp_is_sendonly(&desc));

  testrun(ov_node_push((void **)&desc.attributes, &attr1));
  testrun(ov_node_push((void **)&desc.attributes, &attr2));
  testrun(ov_node_push((void **)&desc.attributes, &attr3));

  testrun(3 == ov_node_count(desc.attributes));
  testrun(ov_sdp_is_sendonly(&desc));
  attr2.key = "2";
  testrun(!ov_sdp_is_sendonly(&desc));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_is_sendrecv() {

  ov_sdp_description desc;
  memset(&desc, 0, sizeof(desc));

  ov_sdp_list attr1 = (ov_sdp_list){.key = "1"};
  ov_sdp_list attr2 = (ov_sdp_list){.key = "sendrecv"};
  ov_sdp_list attr3 = (ov_sdp_list){.key = "3"};

  testrun(!ov_sdp_is_sendrecv(NULL));
  testrun(!ov_sdp_is_sendrecv(&desc));

  testrun(ov_node_push((void **)&desc.attributes, &attr1));
  testrun(ov_node_push((void **)&desc.attributes, &attr2));
  testrun(ov_node_push((void **)&desc.attributes, &attr3));

  testrun(3 == ov_node_count(desc.attributes));
  testrun(ov_sdp_is_sendrecv(&desc));
  attr2.key = "2";
  testrun(!ov_sdp_is_sendrecv(&desc));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_is_inactive() {

  ov_sdp_description desc;
  memset(&desc, 0, sizeof(desc));

  ov_sdp_list attr1 = (ov_sdp_list){.key = "1"};
  ov_sdp_list attr2 = (ov_sdp_list){.key = "inactive"};
  ov_sdp_list attr3 = (ov_sdp_list){.key = "3"};

  testrun(!ov_sdp_is_inactive(NULL));
  testrun(!ov_sdp_is_inactive(&desc));

  testrun(ov_node_push((void **)&desc.attributes, &attr1));
  testrun(ov_node_push((void **)&desc.attributes, &attr2));
  testrun(ov_node_push((void **)&desc.attributes, &attr3));

  testrun(3 == ov_node_count(desc.attributes));
  testrun(ov_sdp_is_inactive(&desc));
  attr2.key = "2";
  testrun(!ov_sdp_is_inactive(&desc));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_get_type() {

  ov_sdp_description desc;
  memset(&desc, 0, sizeof(desc));

  const char *v1 = "one";
  const char *v2 = "two";
  const char *v3 = "three";

  ov_sdp_list attr1 = (ov_sdp_list){.key = "1", .value = v1};
  ov_sdp_list attr2 = (ov_sdp_list){.key = "type", .value = v2};
  ov_sdp_list attr3 = (ov_sdp_list){.key = "3", .value = v3};

  testrun(!ov_sdp_get_type(NULL));
  testrun(!ov_sdp_get_type(&desc));

  testrun(ov_node_push((void **)&desc.attributes, &attr1));
  testrun(ov_node_push((void **)&desc.attributes, &attr2));
  testrun(ov_node_push((void **)&desc.attributes, &attr3));

  testrun(3 == ov_node_count(desc.attributes));
  testrun(v2 == ov_sdp_get_type(&desc));
  attr2.key = "2";
  testrun(!ov_sdp_get_type(&desc));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_get_charset() {

  ov_sdp_description desc;
  memset(&desc, 0, sizeof(desc));

  const char *v1 = "one";
  const char *v2 = "two";
  const char *v3 = "three";

  ov_sdp_list attr1 = (ov_sdp_list){.key = "1", .value = v1};
  ov_sdp_list attr2 = (ov_sdp_list){.key = "charset", .value = v2};
  ov_sdp_list attr3 = (ov_sdp_list){.key = "3", .value = v3};

  testrun(!ov_sdp_get_charset(NULL));
  testrun(!ov_sdp_get_charset(&desc));

  testrun(ov_node_push((void **)&desc.attributes, &attr1));
  testrun(ov_node_push((void **)&desc.attributes, &attr2));
  testrun(ov_node_push((void **)&desc.attributes, &attr3));

  testrun(3 == ov_node_count(desc.attributes));
  testrun(v2 == ov_sdp_get_charset(&desc));
  attr2.key = "2";
  testrun(!ov_sdp_get_charset(&desc));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_get_sdplang() {

  ov_sdp_description desc;
  memset(&desc, 0, sizeof(desc));

  const char *v1 = "one";
  const char *v2 = "two";
  const char *v3 = "three";

  ov_sdp_list attr1 = (ov_sdp_list){.key = "1", .value = v1};
  ov_sdp_list attr2 = (ov_sdp_list){.key = "sdplang", .value = v2};
  ov_sdp_list attr3 = (ov_sdp_list){.key = "3", .value = v3};

  testrun(!ov_sdp_get_sdplang(NULL));
  testrun(!ov_sdp_get_sdplang(&desc));

  testrun(ov_node_push((void **)&desc.attributes, &attr1));
  testrun(ov_node_push((void **)&desc.attributes, &attr2));
  testrun(ov_node_push((void **)&desc.attributes, &attr3));

  testrun(3 == ov_node_count(desc.attributes));
  testrun(v2 == ov_sdp_get_sdplang(&desc));
  attr2.key = "2";
  testrun(!ov_sdp_get_sdplang(&desc));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_get_lang() {

  ov_sdp_description desc;
  memset(&desc, 0, sizeof(desc));

  const char *v1 = "one";
  const char *v2 = "two";
  const char *v3 = "three";

  ov_sdp_list attr1 = (ov_sdp_list){.key = "1", .value = v1};
  ov_sdp_list attr2 = (ov_sdp_list){.key = "lang", .value = v2};
  ov_sdp_list attr3 = (ov_sdp_list){.key = "3", .value = v3};

  testrun(!ov_sdp_get_lang(NULL));
  testrun(!ov_sdp_get_lang(&desc));

  testrun(ov_node_push((void **)&desc.attributes, &attr1));
  testrun(ov_node_push((void **)&desc.attributes, &attr2));
  testrun(ov_node_push((void **)&desc.attributes, &attr3));

  testrun(3 == ov_node_count(desc.attributes));
  testrun(v2 == ov_sdp_get_lang(&desc));
  attr2.key = "2";
  testrun(!ov_sdp_get_lang(&desc));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_get_framerate() {

  ov_sdp_description desc;
  memset(&desc, 0, sizeof(desc));

  const char *v1 = "one";
  const char *v2 = "123";
  const char *v3 = "three";

  ov_sdp_list attr1 = (ov_sdp_list){.key = "1", .value = v1};
  ov_sdp_list attr2 = (ov_sdp_list){.key = "framerate", .value = v2};
  ov_sdp_list attr3 = (ov_sdp_list){.key = "3", .value = v3};

  testrun(0 == ov_sdp_get_framerate(NULL));
  testrun(0 == ov_sdp_get_framerate(&desc));

  testrun(ov_node_push((void **)&desc.attributes, &attr1));
  testrun(ov_node_push((void **)&desc.attributes, &attr2));
  testrun(ov_node_push((void **)&desc.attributes, &attr3));

  testrun(3 == ov_node_count(desc.attributes));
  testrun(123 == ov_sdp_get_framerate(&desc));
  attr2.key = "2";
  testrun(0 == ov_sdp_get_framerate(&desc));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_get_quality() {

  ov_sdp_description desc;
  memset(&desc, 0, sizeof(desc));

  const char *v1 = "one";
  const char *v2 = "1";
  const char *v3 = "three";

  ov_sdp_list attr1 = (ov_sdp_list){.key = "1", .value = v1};
  ov_sdp_list attr2 = (ov_sdp_list){.key = "quality", .value = v2};
  ov_sdp_list attr3 = (ov_sdp_list){.key = "3", .value = v3};

  testrun(0 == ov_sdp_get_quality(NULL));
  testrun(0 == ov_sdp_get_quality(&desc));

  testrun(ov_node_push((void **)&desc.attributes, &attr1));
  testrun(ov_node_push((void **)&desc.attributes, &attr2));
  testrun(ov_node_push((void **)&desc.attributes, &attr3));

  testrun(3 == ov_node_count(desc.attributes));
  testrun(1 == ov_sdp_get_quality(&desc));
  attr2.key = "2";
  testrun(0 == ov_sdp_get_quality(&desc));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_get_orientation() {

  ov_sdp_description desc;
  memset(&desc, 0, sizeof(desc));

  const char *v1 = "one";
  const char *v2 = "two";
  const char *v3 = "three";

  ov_sdp_list attr1 = (ov_sdp_list){.key = "1", .value = v1};
  ov_sdp_list attr2 = (ov_sdp_list){.key = "orientation", .value = v2};
  ov_sdp_list attr3 = (ov_sdp_list){.key = "3", .value = v3};

  testrun(!ov_sdp_get_orientation(NULL));
  testrun(!ov_sdp_get_orientation(&desc));

  testrun(ov_node_push((void **)&desc.attributes, &attr1));
  testrun(ov_node_push((void **)&desc.attributes, &attr2));
  testrun(ov_node_push((void **)&desc.attributes, &attr3));

  testrun(3 == ov_node_count(desc.attributes));
  testrun(v2 == ov_sdp_get_orientation(&desc));
  attr2.key = "2";
  testrun(!ov_sdp_get_orientation(&desc));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_get_fmtp() {

  ov_sdp_description desc;
  memset(&desc, 0, sizeof(desc));

  ov_buffer *buffer1 = ov_buffer_from_string("100 0");
  ov_buffer *buffer2 = ov_buffer_from_string("101 1");
  ov_buffer *buffer3 = ov_buffer_from_string("102 2");

  ov_sdp_list attr1 =
      (ov_sdp_list){.key = "1", .value = (char *)buffer1->start};
  ov_sdp_list attr2 =
      (ov_sdp_list){.key = "fmtp", .value = (char *)buffer2->start};
  ov_sdp_list attr3 =
      (ov_sdp_list){.key = "3", .value = (char *)buffer3->start};

  testrun(!ov_sdp_get_fmtp(NULL, NULL));
  testrun(!ov_sdp_get_fmtp(&desc, NULL));
  testrun(!ov_sdp_get_fmtp(NULL, "102"));

  testrun(ov_node_push((void **)&desc.attributes, &attr1));
  testrun(ov_node_push((void **)&desc.attributes, &attr2));
  testrun(ov_node_push((void **)&desc.attributes, &attr3));

  testrun(3 == ov_node_count(desc.attributes));
  testrun(!ov_sdp_get_fmtp(&desc, "100"));
  testrun(ov_sdp_get_fmtp(&desc, "101"));
  testrun(!ov_sdp_get_fmtp(&desc, "102"));
  testrun(1 == strlen(ov_sdp_get_fmtp(&desc, "101")));
  testrun(0 == strncmp(ov_sdp_get_fmtp(&desc, "101"), "1", 1));
  attr2.key = "2";
  testrun(!ov_sdp_get_fmtp(&desc, "101"));

  attr1.key = "fmtp";
  attr2.key = "fmtp";
  attr3.key = "fmtp";
  testrun(0 == strncmp(ov_sdp_get_fmtp(&desc, "100"), "0", 1));
  testrun(0 == strncmp(ov_sdp_get_fmtp(&desc, "101"), "1", 1));
  testrun(0 == strncmp(ov_sdp_get_fmtp(&desc, "102"), "2", 1));

  testrun(NULL == ov_buffer_free(buffer1));
  testrun(NULL == ov_buffer_free(buffer2));
  testrun(NULL == ov_buffer_free(buffer3));
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_get_rtpmap() {

  ov_sdp_description desc;
  memset(&desc, 0, sizeof(desc));

  ov_buffer *buffer1 = ov_buffer_from_string("100 0");
  ov_buffer *buffer2 = ov_buffer_from_string("101 1");
  ov_buffer *buffer3 = ov_buffer_from_string("102 2");

  ov_sdp_list attr1 =
      (ov_sdp_list){.key = "1", .value = (char *)buffer1->start};
  ov_sdp_list attr2 =
      (ov_sdp_list){.key = "fmtp", .value = (char *)buffer2->start};
  ov_sdp_list attr3 =
      (ov_sdp_list){.key = "rtpmap", .value = (char *)buffer3->start};

  testrun(!ov_sdp_get_rtpmap(NULL, NULL));
  testrun(!ov_sdp_get_rtpmap(&desc, NULL));
  testrun(!ov_sdp_get_rtpmap(NULL, "102"));

  testrun(ov_node_push((void **)&desc.attributes, &attr1));
  testrun(ov_node_push((void **)&desc.attributes, &attr2));
  testrun(ov_node_push((void **)&desc.attributes, &attr3));

  testrun(3 == ov_node_count(desc.attributes));
  testrun(!ov_sdp_get_rtpmap(&desc, "100"));
  testrun(!ov_sdp_get_rtpmap(&desc, "101"));
  testrun(0 == strncmp(ov_sdp_get_rtpmap(&desc, "102"), "2", 1));
  attr3.key = "2";
  testrun(!ov_sdp_get_rtpmap(&desc, "102"));

  attr1.key = "rtpmap";
  attr2.key = "rtpmap";
  attr3.key = "rtpmap";
  testrun(0 == strncmp(ov_sdp_get_rtpmap(&desc, "100"), "0", 1));
  testrun(0 == strncmp(ov_sdp_get_rtpmap(&desc, "101"), "1", 1));
  testrun(0 == strncmp(ov_sdp_get_rtpmap(&desc, "102"), "2", 1));

  testrun(NULL == ov_buffer_free(buffer1));
  testrun(NULL == ov_buffer_free(buffer2));
  testrun(NULL == ov_buffer_free(buffer3));
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_attribute_add() {

  ov_sdp_description desc;
  memset(&desc, 0, sizeof(desc));

  ov_sdp_list *node = NULL;
  testrun(NULL == desc.attributes);

  const char *key1 = "1";
  const char *key2 = "2";
  const char *key3 = "3";

  const char *val2 = "two";
  const char *val3 = "three";

  testrun(!ov_sdp_attribute_add(NULL, NULL, NULL));
  testrun(!ov_sdp_attribute_add(&desc.attributes, NULL, NULL));
  testrun(!ov_sdp_attribute_add(NULL, key1, NULL));

  testrun(ov_sdp_attribute_add(&desc.attributes, key1, NULL));
  testrun(NULL != desc.attributes);
  testrun(1 == ov_node_count(desc.attributes));
  testrun(key1 == desc.attributes->key);

  testrun(ov_sdp_attribute_add(&desc.attributes, key2, val2));
  testrun(NULL != desc.attributes);
  testrun(2 == ov_node_count(desc.attributes));
  node = ov_node_next(desc.attributes);
  testrun(key1 == desc.attributes->key);
  testrun(key2 == node->key);
  testrun(val2 == node->value);

  testrun(ov_sdp_attribute_add(&desc.attributes, key3, val3));
  testrun(NULL != desc.attributes);
  testrun(3 == ov_node_count(desc.attributes));
  node = ov_node_next(desc.attributes);
  testrun(key1 == desc.attributes->key);
  testrun(key2 == node->key);
  testrun(val2 == node->value);
  node = ov_node_next(node);
  testrun(key3 == node->key);
  testrun(val3 == node->value);

  /*
   *      Manual cleanup
   */
  while (desc.attributes) {
    node = ov_node_pop((void **)&desc.attributes);
    node = ov_data_pointer_free(node);
  }

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CLUSTER                                                    #CLUSTER
 *
 *      ------------------------------------------------------------------------
 */

int all_tests() {

  testrun_init();
  testrun_test(test_ov_sdp_attribute_is_set);
  testrun_test(test_ov_sdp_attribute_get);
  testrun_test(test_ov_sdp_attribute_iterate);
  testrun_test(test_ov_sdp_is_recvonly);
  testrun_test(test_ov_sdp_is_sendrecv);
  testrun_test(test_ov_sdp_is_sendonly);
  testrun_test(test_ov_sdp_is_inactive);

  testrun_test(test_ov_sdp_get_orientation);
  testrun_test(test_ov_sdp_get_type);
  testrun_test(test_ov_sdp_get_charset);
  testrun_test(test_ov_sdp_get_sdplang);
  testrun_test(test_ov_sdp_get_lang);
  testrun_test(test_ov_sdp_get_framerate);
  testrun_test(test_ov_sdp_get_quality);

  testrun_test(test_ov_sdp_get_fmtp);
  testrun_test(test_ov_sdp_get_rtpmap);

  testrun_test(test_ov_sdp_attribute_add);

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

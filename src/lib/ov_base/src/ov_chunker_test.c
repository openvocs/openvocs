/***
        ------------------------------------------------------------------------

        Copyright (c) 2023 German Aerospace Center DLR e.V. (GSOC)

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
#include "ov_chunker.c"
#include <../include/ov_random.h>
#include <../include/ov_string.h>
#include <ov_test/ov_test.h>

/*----------------------------------------------------------------------------*/

static int test_ov_chunker_create() {

  ov_chunker *chunker = ov_chunker_create();
  testrun(0 != chunker);

  chunker = ov_chunker_free(chunker);
  testrun(0 == chunker);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_chunker_free() {

  testrun(0 == ov_chunker_free(0));
  testrun(0 == ov_chunker_free(ov_chunker_create()));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static ov_buffer *random_data(size_t octets) {

  ov_buffer *b = ov_buffer_create(octets);
  ov_random_string((char **)&b->start, octets, 0);

  return b;
}

/*----------------------------------------------------------------------------*/

static int test_ov_chunker_add() {

  testrun(!ov_chunker_add(0, 0));

  ov_chunker *c = ov_chunker_create();
  testrun(!ov_chunker_add(c, 0));

  ov_buffer *data = ov_buffer_create(0);
  data->length = 0;

  testrun(ov_chunker_add(c, data));
  data = ov_buffer_free(data);

  data = random_data(12);

  testrun(!ov_chunker_add(0, data));
  testrun(ov_chunker_add(c, data));

  data = ov_buffer_free(data);

  c = ov_chunker_free(c);
  testrun(0 == c);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static bool add_string(ov_chunker *c, char const *str) {

  size_t len = ov_string_len(str);
  ov_buffer *strbuf = ov_buffer_create(len);

  if (0 == strbuf) {
    return false;
  } else {
    memcpy(strbuf->start, str, len);
    strbuf->length = len;

    bool ok = ov_chunker_add(c, strbuf);
    strbuf = ov_buffer_free(strbuf);

    return ok;
  }
}

/*----------------------------------------------------------------------------*/

static bool equals(ov_buffer const *buf, char const *str) {

  size_t len = ov_string_len(str);

  if ((0 == buf) || (0 == str)) {
    return (0 == buf) && (0 == str);
  } else if (buf->length != len) {
    return false;
  } else {
    return 0 == memcmp(buf->start, str, len);
  }
}

/*----------------------------------------------------------------------------*/

uint8_t const *ov_chunker_next_chunk_preview(ov_chunker *self,
                                             size_t num_octets);

static int test_ov_chunker_next_chunk_preview() {

  testrun(0 == ov_chunker_next_chunk_preview(0, 0));

  ov_chunker *chunker = ov_chunker_create();
  testrun(0 != chunker);

  testrun(0 == ov_chunker_next_chunk_preview(chunker, 0));
  testrun(0 == ov_chunker_next_chunk_preview(chunker, 3));

  testrun(add_string(chunker, "123"));

  testrun(0 == ov_chunker_next_chunk_preview(chunker, 4));
  uint8_t const *data = ov_chunker_next_chunk_preview(chunker, 3);
  testrun(0 != data);

  testrun(0 == memcmp("123", data, 3));
  data = 0;

  testrun(add_string(chunker, "456"));
  data = ov_chunker_next_chunk_preview(chunker, 3);
  testrun(0 != data);

  testrun(0 == memcmp("123", data, 3));

  data = ov_chunker_next_chunk_preview(chunker, 4);
  testrun(0 != data);

  testrun(0 == memcmp("1234", data, 4));

  data = ov_chunker_next_chunk_preview(chunker, 6);
  testrun(0 != data);

  testrun(0 == memcmp("123456", data, 6));

  ov_buffer *rec = ov_chunker_next_chunk(chunker, 4);
  testrun(equals(rec, "1234"));
  rec = ov_buffer_free(rec);

  testrun(0 == ov_chunker_next_chunk_preview(chunker, 3));

  data = ov_chunker_next_chunk_preview(chunker, 2);
  testrun(0 != data);

  testrun(0 == memcmp("56", data, 2));

  chunker = ov_chunker_free(chunker);
  testrun(0 == chunker);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static bool next_is(ov_chunker *c, size_t octets, char const *ref) {

  if ((0 == c) || (0 == ref)) {
    return false;

  } else {

    ov_buffer *b = ov_chunker_next_chunk(c, octets);
    bool ok = equals(b, ref);
    b = ov_buffer_free(b);
    return ok;
  }
}

/*----------------------------------------------------------------------------*/

static int test_ov_chunker_next_chunk() {

  testrun(0 == ov_chunker_next_chunk(0, 0));

  ov_chunker *c = ov_chunker_create();
  testrun(0 == ov_chunker_next_chunk(c, 0));
  testrun(0 == ov_chunker_next_chunk(0, 10));

  testrun(add_string(c, "abc"));

  // Not enough octets yet
  testrun(0 == ov_chunker_next_chunk(0, 10));

  testrun(0 == ov_chunker_next_chunk(c, 10));

  ov_buffer *rec = ov_chunker_next_chunk(c, 3);
  testrun(equals(rec, "abc"));
  rec = ov_buffer_free(rec);

  testrun(add_string(c, "alfred"));
  testrun(0 == ov_chunker_next_chunk(0, 10));
  rec = ov_chunker_next_chunk(c, 3);
  testrun(equals(rec, "alf"));
  rec = ov_buffer_free(rec);

  testrun(add_string(c, "li"));
  testrun(0 == ov_chunker_next_chunk(c, 7));

  testrun(add_string(c, "ch"));
  rec = ov_chunker_next_chunk(c, 7);
  testrun(equals(rec, "redlich"));
  rec = ov_buffer_free(rec);

  testrun(add_string(c, "Cuando sali de la habana, valgame dios"
                        "Nadie me ha visto salir sino fui yo"));

  testrun(0 == ov_chunker_next_chunk(c, 722));

  testrun(next_is(c, 7, "Cuando "));
  testrun(next_is(c, 19, "sali de la habana, "));

  testrun(add_string(c, "Y una linda guachinanga como una flor"));

  testrun(0 == ov_chunker_next_chunk(c, 722));

  testrun(add_string(c, "Se vino detrás de mí, que si señor"
                        "Si a tu ventana llega una paloma"
                        "Trátala con cariño que es mi persona"));

  testrun(0 == ov_chunker_next_chunk(c, 722));

  testrun(add_string(c, "Cuéntale tus amores bien de mi vida"));

  testrun(0 == ov_chunker_next_chunk(c, 722));

  testrun(add_string(c, "Corónala de flores que es cosa mía"
                        "Ay chinita que si, ay que dame tu amor"
                        "Ay que vente conmigo chinita"
                        "A donde vivo yo"
                        "Y una linda guachinanga como una flor"
                        "Se vino detrás de mí, que si señor"
                        "Si a tu ventana llega una paloma"
                        "Trátala con cariño que es mi persona"
                        "Cuéntale tus amores bien de mi vida"));

  testrun(0 == ov_chunker_next_chunk(c, 722));

  testrun(add_string(c, "Corónala de flores que es cosa mía"));

  testrun(0 == ov_chunker_next_chunk(c, 722));

  testrun(add_string(c, "Ay chinita que si, ay que dame tu amor"
                        "Ay que vente conmigo chinita"
                        "A donde vivo yo"
                        "Ay chinita que si, ay que dame tu amor"
                        "Ay que vente conmigo chinita"
                        "A donde vivo yo"));

  testrun(0 == ov_chunker_next_chunk(c, 723));

  testrun(next_is(c, 722,
                  "valgame dios"
                  "Nadie me ha visto salir sino fui yo"
                  "Y una linda guachinanga como una flor"
                  "Se vino detrás de mí, que si señor"
                  "Si a tu ventana llega una paloma"
                  "Trátala con cariño que es mi persona"
                  "Cuéntale tus amores bien de mi vida"
                  "Corónala de flores que es cosa mía"
                  "Ay chinita que si, ay que dame tu amor"
                  "Ay que vente conmigo chinita"
                  "A donde vivo yo"
                  "Y una linda guachinanga como una flor"
                  "Se vino detrás de mí, que si señor"
                  "Si a tu ventana llega una paloma"
                  "Trátala con cariño que es mi persona"
                  "Cuéntale tus amores bien de mi vida"
                  "Corónala de flores que es cosa mía"
                  "Ay chinita que si, ay que dame tu amor"
                  "Ay que vente conmigo chinita"
                  "A donde vivo yo"
                  "Ay chinita que si, ay que dame tu amor"
                  "Ay que vente conmigo chinita"
                  "A donde vivo yo"));

  c = ov_chunker_free(c);
  testrun(0 == c);

  return testrun_log_success();
}

/*****************************************************************************
                           ov_chunker_next_chunk_raw
 ****************************************************************************/

static bool raw_equals(uint8_t const *buffer, char const *ref) {

  char const *buf = (char const *)buffer;

  if ((0 == buf) || (0 == ref)) {

    return (void *)buffer == (void *)ref;

  } else {

    bool equals = true;

    for (size_t i = 0; 0 != ref[i]; ++i) {
      equals = equals && (ref[i] == buf[i]);
    }

    return equals;
  }
}

/*----------------------------------------------------------------------------*/

static bool next_raw_is(ov_chunker *chunker, size_t expected_octets,
                        char const *ref) {

  uint8_t *buf = calloc(1, expected_octets);

  bool ok = ov_chunker_next_chunk_raw(chunker, expected_octets, buf) &&
            raw_equals(buf, ref);

  buf = ov_free(buf);

  return ok;
}

/*----------------------------------------------------------------------------*/

static int test_ov_chunker_next_chunk_raw() {

  bool ov_chunker_next_chunk_raw(ov_chunker * self, size_t num_octets,
                                 uint8_t * dest);

  testrun(!ov_chunker_next_chunk_raw(0, 0, 0));

  ov_chunker *c = ov_chunker_create();
  testrun(0 != c);

  testrun(!ov_chunker_next_chunk_raw(c, 0, 0));
  testrun(!ov_chunker_next_chunk_raw(c, 10, 0));

  uint8_t buffer[500] = {0};

  testrun(!ov_chunker_next_chunk_raw(c, 0, buffer));
  testrun(0 == ov_chunker_next_chunk_raw(c, 10, buffer));

  testrun(add_string(c, "abc"));

  // Not enough octets yet
  testrun(!ov_chunker_next_chunk_raw(c, 10, buffer));
  testrun(ov_chunker_next_chunk_raw(c, 3, buffer));
  testrun(raw_equals(buffer, "abc"));

  testrun(add_string(c, "alfred"));
  testrun(ov_chunker_next_chunk_raw(c, 3, buffer));
  testrun(raw_equals(buffer, "alf"));

  testrun(add_string(c, "li"));
  testrun(!ov_chunker_next_chunk_raw(c, 7, buffer));

  testrun(add_string(c, "ch"));
  testrun(ov_chunker_next_chunk_raw(c, 7, buffer));
  testrun(raw_equals(buffer, "redlich"));

  testrun(add_string(c, "Cuando sali de la habana, valgame dios"
                        "Nadie me ha visto salir sino fui yo"));

  testrun(!ov_chunker_next_chunk_raw(c, 722, buffer));

  testrun(next_raw_is(c, 7, "Cuando "));
  testrun(next_raw_is(c, 19, "sali de la habana, "));

  testrun(add_string(c, "Y una linda guachinanga como una flor"));

  testrun(!ov_chunker_next_chunk_raw(c, 722, buffer));

  testrun(add_string(c, "Se vino detrás de mí, que si señor"
                        "Si a tu ventana llega una paloma"
                        "Trátala con cariño que es mi persona"));

  testrun(!ov_chunker_next_chunk_raw(c, 722, buffer));

  testrun(add_string(c, "Cuéntale tus amores bien de mi vida"));

  testrun(!ov_chunker_next_chunk_raw(c, 722, buffer));

  testrun(add_string(c, "Corónala de flores que es cosa mía"
                        "Ay chinita que si, ay que dame tu amor"
                        "Ay que vente conmigo chinita"
                        "A donde vivo yo"
                        "Y una linda guachinanga como una flor"
                        "Se vino detrás de mí, que si señor"
                        "Si a tu ventana llega una paloma"
                        "Trátala con cariño que es mi persona"
                        "Cuéntale tus amores bien de mi vida"));

  testrun(!ov_chunker_next_chunk_raw(c, 722, buffer));

  testrun(add_string(c, "Corónala de flores que es cosa mía"));

  testrun(!ov_chunker_next_chunk_raw(c, 722, buffer));

  testrun(add_string(c, "Ay chinita que si, ay que dame tu amor"
                        "Ay que vente conmigo chinita"
                        "A donde vivo yo"
                        "Ay chinita que si, ay que dame tu amor"
                        "Ay que vente conmigo chinita"
                        "A donde vivo yo"));

  testrun(!ov_chunker_next_chunk_raw(c, 723, buffer));

  testrun(next_raw_is(c, 722,
                      "valgame dios"
                      "Nadie me ha visto salir sino fui yo"
                      "Y una linda guachinanga como una flor"
                      "Se vino detrás de mí, que si señor"
                      "Si a tu ventana llega una paloma"
                      "Trátala con cariño que es mi persona"
                      "Cuéntale tus amores bien de mi vida"
                      "Corónala de flores que es cosa mía"
                      "Ay chinita que si, ay que dame tu amor"
                      "Ay que vente conmigo chinita"
                      "A donde vivo yo"
                      "Y una linda guachinanga como una flor"
                      "Se vino detrás de mí, que si señor"
                      "Si a tu ventana llega una paloma"
                      "Trátala con cariño que es mi persona"
                      "Cuéntale tus amores bien de mi vida"
                      "Corónala de flores que es cosa mía"
                      "Ay chinita que si, ay que dame tu amor"
                      "Ay que vente conmigo chinita"
                      "A donde vivo yo"
                      "Ay chinita que si, ay que dame tu amor"
                      "Ay que vente conmigo chinita"
                      "A donde vivo yo"));

  c = ov_chunker_free(c);
  testrun(0 == c);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_chunker_remainder() {

  testrun(0 == ov_chunker_remainder(0));

  ov_chunker *c = ov_chunker_create();
  testrun(0 == ov_chunker_remainder(c));

  testrun(add_string(c, "abc"));
  testrun(add_string(c, "defgh"));

  ov_buffer *r = ov_chunker_remainder(c);

  testrun(0 != r);
  testrun(r->length == strlen("abcdefgh"));
  testrun(0 == memcmp(r->start, "abcdefgh", r->length));

  r = ov_buffer_free(r);

  testrun(add_string(c, "defgh"));
  testrun(add_string(c, "abc"));

  ov_buffer_free(ov_chunker_next_chunk(c, 3));

  r = ov_chunker_remainder(c);

  testrun(0 != r);
  testrun(r->length == strlen("ghabc"));
  testrun(0 == memcmp(r->start, "ghabc", r->length));

  r = ov_buffer_free(r);

  c = ov_chunker_free(c);
  testrun(0 == c);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_chunker_enable_caching() {

  void ov_chunker_enable_caching();

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_chunker", test_ov_chunker_create, test_ov_chunker_free,
            test_ov_chunker_add, test_ov_chunker_next_chunk_preview,
            test_ov_chunker_next_chunk, test_ov_chunker_next_chunk_raw,
            test_ov_chunker_remainder, test_ov_chunker_enable_caching);

/*----------------------------------------------------------------------------*/

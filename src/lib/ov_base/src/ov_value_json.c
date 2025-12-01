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
        @file           ov_value_json.c

        @author         Michael J. Beer, DLR/GSOC
        @author         Markus Töpfer

        @date           2021-04-01


        ------------------------------------------------------------------------
*/
#include "../include/ov_value_json.h"

#include "../include/ov_utils.h"
#include <../include/ov_buffer.h>
#include <../include/ov_convert.h>
#include <ctype.h>
#include <stdio.h>

#include "../include/ov_utf8.h"
#include "../include/ov_value.h"

/*----------------------------------------------------------------------------*/

static char const WHITESPACE_CHARS[] = " \n\t\r";

/*----------------------------------------------------------------------------*/

static char const *skip_chars(char const *in, char const *skip_chars,
                              size_t len) {

  OV_ASSERT(0 != in);
  OV_ASSERT(0 != skip_chars);

  size_t i = 0;

  for (; i < len; ++i) {

    if (0 == strchr(skip_chars, in[i]))
      break;
  }

  return in + i;
}

/*----------------------------------------------------------------------------*/

typedef ov_value *(*ParseFunc)(char const *in, size_t len,
                               char const **remainder);

/*----------------------------------------------------------------------------*/

static ov_value *parse_next_token(char const *in, size_t len,
                                  char const **remainder);

/*----------------------------------------------------------------------------*/

#define CHECK_INPUT(in, len, min_len, remainder)                               \
  OV_ASSERT(0 != in);                                                          \
  OV_ASSERT(0 != remainder);                                                   \
  if (len < min_len)                                                           \
    goto error;

/*----------------------------------------------------------------------------*/

static ov_value *parse_partial(const char *valid, char const *in, size_t len,
                               char const **remainder) {

  OV_ASSERT(valid);
  OV_ASSERT(in);
  OV_ASSERT(len > 0);
  OV_ASSERT(remainder);

  size_t pos = 0;
  for (pos = 0; pos < len; pos++) {

    if (in[pos] != valid[pos])
      break;
  }

  *remainder = in + pos;
  return NULL;
}

/*----------------------------------------------------------------------------*/

static ov_value *parse_null(char const *in, size_t len,
                            char const **remainder) {

  CHECK_INPUT(in, len, 1, remainder);

  if (len < 4)
    return parse_partial("null", in, len, remainder);

  if (0 != memcmp(in, "null", 4))
    goto error;

  *remainder = in + 4;

  return ov_value_null();

error:

  if (remainder)
    *remainder = in;

  return 0;
}

/*----------------------------------------------------------------------------*/

static ov_value *parse_true(char const *in, size_t len,
                            char const **remainder) {

  CHECK_INPUT(in, len, 1, remainder);

  if (len < 4)
    return parse_partial("true", in, len, remainder);

  if (0 != memcmp(in, "true", 4))
    goto error;

  *remainder = in + 4;

  return ov_value_true();

error:

  if (remainder)
    *remainder = in;

  return 0;
}

/*----------------------------------------------------------------------------*/

static ov_value *parse_false(char const *in, size_t len,
                             char const **remainder) {

  CHECK_INPUT(in, len, 1, remainder);

  if (len < 5)
    return parse_partial("false", in, len, remainder);

  if (0 != memcmp(in, "false", 5))
    goto error;

  *remainder = in + 5;

  return ov_value_false();

error:

  if (remainder)
    *remainder = in;

  return 0;
}

/*****************************************************************************
                                     String
 ****************************************************************************/

static bool unescape_json(const uint8_t *start, size_t len, ov_buffer *result) {

  /* RFC 8259 7. Strings

  The representation of strings is similar to conventions used in the C
  family of programming languages.  A string begins and ends with
  quotation marks.  All Unicode characters may be placed within the
  quotation marks, except for the characters that MUST be escaped:
  quotation mark, reverse solidus, and the control characters (U+0000
  through U+001F).

  Any character may be escaped.  If the character is in the Basic
  Multilingual Plane (U+0000 through U+FFFF), then it may be
  represented as a six-character sequence: a reverse solidus, followed
  by the lowercase letter u, followed by four hexadecimal digits that
  encode the character's code point.  The hexadecimal letters A through
  F can be uppercase or lowercase.  So, for example, a string
  containing only a single reverse solidus character may be represented
  as "\u005C".

  Alternatively, there are two-character sequence escape
  representations of some popular characters.  So, for example, a
  string containing only a single reverse solidus character may be
  represented more compactly as "\\".

  To escape an extended character that is not in the Basic Multilingual
  Plane, the character is represented as a 12-character sequence,
  encoding the UTF-16 surrogate pair.  So, for example, a string
  containing only the G clef character (U+1D11E) may be represented as
  "\uD834\uDD1E".

      string = quotation-mark *char quotation-mark

      char = unescaped /
        escape (
            %x22 /          ; "    quotation mark  U+0022
            %x5C /          ; \    reverse solidus U+005C
            %x2F /          ; /    solidus         U+002F
            %x62 /          ; b    backspace       U+0008
            %x66 /          ; f    form feed       U+000C
            %x6E /          ; n    line feed       U+000A
            %x72 /          ; r    carriage return U+000D
            %x74 /          ; t    tab             U+0009
            %x75 4HEXDIG )  ; uXXXX                U+XXXX

      escape = %x5C              ; \

      quotation-mark = %x22      ; "

      unescaped = %x20-21 / %x23-5B / %x5D-10FFFF

  */
  OV_ASSERT(start);
  OV_ASSERT(result);

  if (!ov_buffer_clear(result))
    goto error;

  if (0 == len)
    return true;

  /* RFC 8259 specifies UTF8 as basis,
   * \uXXXX escaped or on word byte input level! */

  if (!ov_utf8_validate_sequence(start, len))
    goto error;

  /* Expected input is the string between the quotes (no quotes contained) */
  if (start[0] == '"')
    goto error;

  size_t check = 0;
  uint8_t *next = result->start;

  uint64_t codepoint_utf8 = 0;
  uint64_t bytes = 0;

  while (check < len) {

    if (start[check] != 0x5C) {

      /* something non escaped
       *
       * NOTE any valid UTF8 bytes sequence cannot become
       * smaller 0x80 except of UTF8 1 byte sequences (ascii),
       * which is actually the byte under test here! */

      if (start[check] < 0x1F)
        goto error;

      if (start[check] == '"')
        goto error;

      *next = start[check];

      next++;
      check++;

      continue;
    }

    // ensure we do not overparse
    if (check == len - 1)
      goto error;

    switch (start[check + 1]) {

    case 0x22: // "    quotation mark  U+0022

      *next = 0x22;
      next++;
      check += 2;
      break;

    case 0x5C: // \    reverse solidus U+005C

      *next = 0x5C;
      next++;
      check += 2;
      break;

    case 0x2F: // /    solidus         U+002F

      *next = 0x2F;
      next++;
      check += 2;
      break;

    case 0x62: // b    backspace       U+0008

      *next = '\b';
      next++;
      check += 2;
      break;

    case 0x66: // f    form feed       U+000C

      *next = '\f';
      next++;
      check += 2;
      break;

    case 0x6E: // n    line feed       U+000A

      *next = '\n';
      next++;
      check += 2;
      break;

    case 0x72: // r    carriage return U+000D

      *next = '\r';
      next++;
      check += 2;
      break;

    case 0x74: // t    tab             U+0009

      *next = '\t';
      next++;
      check += 2;
      break;

    case 0x75: // uXXXX                U+XXXX

      if (!ov_convert_hex_string_to_uint64((char *)start + check + 2, 4,
                                           &codepoint_utf8))
        goto error;

      if (!ov_utf8_encode_code_point(codepoint_utf8, &next, 4, &bytes))
        goto error;

      next += bytes;
      check += 6;
      break;

    default:

      // invalid, so the whole string is invalid
      goto error;
    }
  }

  // set the length of the encoded buffer
  result->length = next - result->start;

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static ov_value *parse_string(char const *in, size_t len,
                              char const **remainder) {

  ov_buffer *buf = NULL;
  ov_value *val = NULL;

  CHECK_INPUT(in, len, 1, remainder);

  uint8_t *start = (uint8_t *)in;
  size_t check = 0;

  if (start[0] != '"')
    goto error;

  start++;
  len--;

  while (check < len) {

    // check for escape sign backslash \ (0x5C)
    if (start[check] == 0x5C) {

      // check sign following backslash
      if (check == len - 1)
        goto matching;

      switch (start[check + 1]) {
      case 0x22: // "    quotation mark  U+0022
      case 0x5C: // \    reverse solidus U+005C
      case 0x2F: // /    solidus         U+002F
      case 0x62: // b    backspace       U+0008
      case 0x66: // f    form feed       U+000C
      case 0x6E: // n    line feed       U+000A
      case 0x72: // r    carriage return U+000D
      case 0x74: // t    tab             U+0009

        // one byte escape, skip checking next
        if (check + 2 > len)
          goto matching;

        check += 2;
        break;

      case 0x75: // uXXXX                U+XXXX

        if (check + 6 > len)
          goto matching;

        // 4 byte unicode escape,
        // skip checking \uXXXX sequence
        check += 6;

        break;

      default:

        // invalid, escaped char not in JSON spec
        *remainder = in + 2 + check;
        return NULL;
      }

    } else if (start[check] < 0x1F) {

      // invalid, unescaped control character
      *remainder = in + 1 + check;
      return NULL;

    } else if (start[check] == 0x22) {

      // end of string

      /* We create a buffer capable to hold any content as UTF8 content.
       * UTF8 is encoded as /uXXXX in the Basic
       * Multilingual Plane (U+0000 through U+FFFF)
       * as well as /uXXXX/uXXXX if outside of the Multilingual Plane,
       * encoding is 6 bytes for a 2 byte UTF8 char and 12 bytes for
       * some 4 byte UTF char. Using a buffer capable to hold the length
       * of the JSON input is definitely big enough to hold the length of
       * some UTF8 escaped sequence in JSON */

      buf = ov_buffer_create(check + 1);

      if (!unescape_json(start, check, buf))
        goto error;

      val = ov_value_string((char *)buf->start);
      buf = ov_buffer_free(buf);

      // offset closing quote
      *remainder = in + check + 2;
      return val;
      break;

    } else {
      check++;
    }
  }

matching:
  *remainder = in + 1 + len;
  return NULL;

error:
  ov_buffer_free(buf);
  ov_value_free(val);

  if (remainder)
    *remainder = in;

  return NULL;
}

/*****************************************************************************
                                     Number
 ****************************************************************************/

static ov_value *parse_number(char const *in, size_t len,
                              char const **remainder) {

  ov_buffer *buf = NULL;

  CHECK_INPUT(in, len, 1, remainder);

  // We need to zero-terminate the 'in' content
  // buffers are cached, thus refrain from using plain char *

  /*  UINT64_t max digits: 20
   *  DOUBLE max digits: 15
   *
   *  We limit strtod to a /0 terminated input of MAX amount of digits.
   */

  size_t max_digits = 20;

  /* This maybe OK for strtod, but is not JSON spec, so rule out here */
  if (in[0] == '.')
    goto error;

  buf = ov_buffer_create(max_digits + 1);

  if (len < max_digits) {

    memcpy(buf->start, in, len);
    buf->start[len + 1] = 0;

  } else {

    memcpy(buf->start, in, max_digits);
    buf->start[max_digits + 1] = 0;
  }

  char *rem = 0;

  double number = strtod((char const *)buf->start, &rem);

  if (rem == (char const *)buf->start)
    goto error;

  ptrdiff_t remainder_offset = 0;

  if (rem) {

    remainder_offset = rem - (char *)buf->start;

  } else {

    remainder_offset = max_digits + 1;
  }

  *remainder = in + remainder_offset;

  /* handle something like "1e" "1e-" "1e+" as imcomplete but valid content */

  switch (len - remainder_offset) {

  case 0:
    break;

  case 1:

    /* allow e as valid charater */

    if ((rem[0] == 'e') || (rem[0] == 'E')) {
      *remainder = in + remainder_offset + 1;
      buf = ov_buffer_free(buf);
      return NULL;
    }

    break;

  case 2:

    /* allow e followed by +/- as valid character */

    if ((rem[0] == 'e') || (rem[0] == 'E')) {

      if ((rem[1] == '-') || (rem[1] == '+')) {

        *remainder = in + remainder_offset + 2;
        buf = ov_buffer_free(buf);
        return NULL;

      } else if (0 == isdigit(rem[1])) {
        goto error;
      }
    }

    break;

  case 3:

    /* allow e followed by +/- as valid character + some digit */

    if ((rem[0] == 'e') || (rem[0] == 'E')) {

      if ((rem[1] == '-') || (rem[1] == '+')) {

        if (0 == isdigit(rem[2]))
          goto error;

      } else {
        goto error;
      }
    }

    break;

  default:
    break;
  }

  ov_value *result = ov_value_number(number);

  buf = ov_buffer_free(buf);
  OV_ASSERT(0 == buf);

  return result;

error:

  buf = ov_buffer_free(buf);
  OV_ASSERT(0 == buf);

  if (remainder)
    *remainder = in;

  return 0;
}

/*****************************************************************************
                                      LIST
 ****************************************************************************/

static ov_value *parse_list(char const *in, size_t len,
                            char const **remainder) {

  ov_value *list = NULL;
  char const *ptr = in;

  CHECK_INPUT(in, len, 1, remainder);

  bool require_comma = false;

  // Skip initial '['
  if (ptr[0] != '[')
    goto error;

  ptr++;

  if (len == 1)
    goto error;

  list = ov_value_list(0);

  while (0 < (len - (ptr - in))) {

    size_t new_len = len - (ptr - in);

    ptr = skip_chars(ptr, WHITESPACE_CHARS, new_len);

    new_len = len - (ptr - in);

    if (0 == new_len)
      goto error;

    if (']' == *ptr) {

      /* only return a list,
       * when parsing for the whole list is completed */

      *remainder = ptr + 1;
      return list;
    }

    if (require_comma && (',' != *ptr))
      goto error;

    if (require_comma) {
      ++ptr;
      --new_len;
    }

    if (0 == new_len)
      goto error;

    ov_value *entry = parse_next_token(ptr, new_len, &ptr);

    if (!entry)
      goto error;

    if (!ov_value_list_push(list, entry)) {

      entry = ov_value_free(entry);
      goto error;
    }

    // We parsed at least one entry - next entry must be preceded by a comma
    require_comma = true;
  };

error:

  list = ov_value_free(list);
  OV_ASSERT(0 == list);

  if (remainder)
    *remainder = ptr;

  return 0;
}

/*****************************************************************************
                                     OBJECT
 ****************************************************************************/

typedef struct {

  ov_value *key;
  ov_value *value;

} kv_pair;

/*----------------------------------------------------------------------------*/

static kv_pair parse_kv_pair(char const *in, size_t len,
                             char const **remainder) {

  kv_pair pair = {0};

  char const *ptr = in;

  CHECK_INPUT(in, len, 1, remainder);

  pair.key = parse_next_token(in, len, &ptr);

  if (0 == ov_value_get_string(pair.key))
    goto error;

  OV_ASSERT(0 != ptr);

  size_t new_len = len - (ptr - in);

  ptr = skip_chars(ptr, WHITESPACE_CHARS, new_len);

  new_len = len - (ptr - in);

  if (0 == new_len)
    goto error;

  if (':' != *ptr)
    goto error;

  ptr++;
  new_len--;

  if (0 == new_len)
    goto error;

  ov_value *value = parse_next_token(ptr, new_len, &ptr);
  if (!value)
    goto error;

  pair.value = value;

  *remainder = ptr;
  return pair;

error:

  pair.key = ov_value_free(pair.key);
  pair.value = ov_value_free(pair.value);
  OV_ASSERT(0 == pair.key);
  OV_ASSERT(0 == pair.value);

  if (remainder)
    *remainder = ptr;

  return pair;
}

/*----------------------------------------------------------------------------*/

static ov_value *parse_object(char const *in, size_t len,
                              char const **remainder) {

  ov_value *object = NULL;

  CHECK_INPUT(in, len, 1, remainder);

  bool require_comma = false;

  char const *ptr = in;

  if (ptr[0] != '{')
    goto error;

  ptr++;

  if (len == 1)
    goto error;

  object = ov_value_object();

  while (0 < (len - (ptr - in))) {

    size_t new_len = len - (ptr - in);

    ptr = skip_chars(ptr, WHITESPACE_CHARS, new_len);

    new_len = len - (ptr - in);

    if (0 == new_len)
      goto error;

    if ('}' == *ptr) {

      /* only return an object,
       * when parsing for the whole object is completed */

      *remainder = ptr + 1;
      return object;
    }

    if (require_comma && (',' != *ptr))
      goto error;

    if (require_comma) {
      ++ptr;
      --new_len;
    }

    if (0 == new_len)
      goto error;

    kv_pair pair = parse_kv_pair(ptr, new_len, &ptr);

    char const *key_str = ov_value_get_string(pair.key);

    if (NULL == key_str)
      goto error;

    if (NULL == pair.value)
      goto error;

    if (0 != ov_value_object_set(object, key_str, pair.value)) {

      pair.key = ov_value_free(pair.key);
      pair.value = ov_value_free(pair.value);

      goto error;
    }

    key_str = 0;

    pair.key = ov_value_free(pair.key);
    pair.value = 0;

    // We parsed at least one entry - next entry must be preceded by a comma
    require_comma = true;
  };

error:

  object = ov_value_free(object);
  OV_ASSERT(0 == object);

  if (remainder)
    *remainder = ptr;

  return 0;
}

/*****************************************************************************
                                  Main parser
 ****************************************************************************/

static ParseFunc get_parse_func_for(char c) {

  switch (c) {

  case 'n':
    return parse_null;
  case 't':
    return parse_true;
  case 'f':
    return parse_false;
  case '"':
    return parse_string;
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9':
  case '0':
  case '-':
  case '+':
    return parse_number;
  case '[':
    return parse_list;

  case '{':
    return parse_object;

  default:
    return 0;
  };

  return 0;
}

/*----------------------------------------------------------------------------*/

static ov_value *parse_next_token(char const *in, size_t len,
                                  char const **remainder) {

  OV_ASSERT(0 != in);
  OV_ASSERT(0 != remainder);

  char const *start_token = skip_chars(in, WHITESPACE_CHARS, len);

  size_t new_len = len - (start_token - in);

  /* We skipped valid whitespace, so the content is matching */

  if (0 == new_len) {
    *remainder = in + len;
    goto error;
  }

  ParseFunc func = get_parse_func_for(*start_token);

  if (0 == func)
    goto error;

  return func(start_token, new_len, remainder);

error:

  return 0;
}

/*----------------------------------------------------------------------------*/

ov_value *ov_value_parse_json(ov_buffer const *in, char const **remainder) {

  if ((0 == in) || (0 == in->length) || (0 == in->start))
    goto error;

  char const *rem = 0;

  ov_value *result =
      parse_next_token((char const *)in->start, in->length, &rem);

  if (0 != remainder)
    *remainder = rem;

  return result;

error:

  return 0;
}

/*----------------------------------------------------------------------------*/

ov_value *ov_value_from_json(ov_buffer const *in) {

  const char *next = NULL;
  ov_value *value = ov_value_parse_json(in, &next);

  if (!value)
    goto error;

  if ((uint8_t *)next != in->start + in->length)
    goto error;

  return value;
error:
  ov_value_free(value);
  return NULL;
}

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
        @file           ov_uri.c
        @author         Markus Töpfer

        @date           2020-01-03

        @ingroup        ov_uri

        @brief          Implementation of


        ------------------------------------------------------------------------
*/
#include "../../include/ov_uri.h"

#include <arpa/inet.h>
#include <ctype.h>
#include <limits.h>

#include "../../include/ov_config_keys.h"
#include "../../include/ov_data_function.h"
#include "../../include/ov_memory_pointer.h"

/*
 *      ------------------------------------------------------------------------
 *
 *      #PRIVATE
 *
 *      ------------------------------------------------------------------------
 */

static bool chars_are_percent_encoded(const char *buffer, size_t size) {

  if (!buffer || size < 3)
    return false;

  if (buffer[0] != '%')
    return false;

  if (!isxdigit(buffer[1]))
    return false;

  if (!isxdigit(buffer[2]))
    return false;

  return true;
}

/*----------------------------------------------------------------------------*/

static bool char_is_unreserved(char byte) {

  // ALPHA NUMMERIC
  if (isalnum(byte))
    return true;

  switch (byte) {

  case '-':
  case '.':
  case '_':
  case '~':
    return true;
  }

  return false;
}

/*----------------------------------------------------------------------------*/

static bool char_is_reserved_general_delimiter(char byte) {

  switch (byte) {

  case ':':
  case '/':
  case '?':
  case '#':
  case '[':
  case ']':
  case '@':
    return true;
  }

  return false;
}

/*----------------------------------------------------------------------------*/

static bool char_is_reserved_sub_delimiter(char byte) {

  switch (byte) {

  case '!':
  case '$':
  case '&':
  case '\'':
  case '(':
  case ')':
  case '*':
  case '+':
  case ',':
  case ';':
  case '=':
    return true;
  }

  return false;
}

/*----------------------------------------------------------------------------*/

static bool char_is_reserved(char byte) {

  if (char_is_reserved_general_delimiter(byte))
    return true;

  if (char_is_reserved_sub_delimiter(byte))
    return true;

  return false;
}
/*----------------------------------------------------------------------------*/

static bool chars_are_allowed(const char *buffer, size_t length) {

  if (!buffer || length < 1)
    return false;

  for (size_t i = 0; i < length; i++) {

    if (buffer[i] == '%') {

      if ((length - i) < 3)
        return false;

      if (!chars_are_percent_encoded(buffer + i, 3))
        return false;

      i = i + 2;

    } else {

      if (!char_is_reserved(buffer[i]))
        if (!char_is_unreserved(buffer[i]))
          return false;
    }
  }

  return true;
}

/*----------------------------------------------------------------------------*/

static bool char_is_scheme(char byte) {

  switch (byte) {

  case '+':
  case '-':
  case '.':
    return true;
  }

  if (isalnum(byte))
    return true;

  return false;
}

/*----------------------------------------------------------------------------*/

static bool chars_are_scheme(const char *buffer, size_t length) {

  if (!buffer || length < 1)
    return false;

  if (!isalpha(buffer[0]))
    return false;

  switch (buffer[length]) {

  case 0:
  case ':':
    break;
  default:
    return false;
  }

  for (size_t i = 0; i < length; i++) {

    if (!char_is_scheme(buffer[i]))
      return false;
  }

  return true;
}

/*----------------------------------------------------------------------------*/

static bool chars_are_user(const char *buffer, size_t length) {

  if (!buffer || length < 1)
    return false;

  for (size_t i = 0; i < length; i++) {

    if (buffer[i] == '%') {

      if ((length - i) < 3)
        return false;

      if (!chars_are_percent_encoded(buffer + i, 3))
        return false;

      i = i + 2;
      continue;
    }

    if (buffer[i] == ':')
      continue;

    if (char_is_unreserved(buffer[i]))
      continue;

    if (char_is_reserved_sub_delimiter(buffer[i]))
      continue;

    return false;
  }

  return true;
}

/*----------------------------------------------------------------------------*/

static bool chars_are_ip(int type, const char *buffer, size_t length) {

  if (!buffer || length < 2)
    return false;

  if (type != AF_INET)
    if (type != AF_INET6)
      return false;

  char terminated[length + 1];
  memset(terminated, 0, length + 1);

  struct sockaddr_storage sa;
  int r = 0;

  if (!strncpy(terminated, buffer, length))
    return false;

  r = inet_pton(type, terminated, &sa);
  if (r != 1)
    return false;

  return true;
}

/*----------------------------------------------------------------------------*/

static bool chars_are_ip_future(const char *buffer, size_t length) {

  if (!buffer || length < 4)
    return false;

  if (tolower(buffer[0]) != 'v')
    return false;

  if (!isxdigit(buffer[1]))
    return false;

  if (buffer[2] != '.')
    return false;

  for (size_t i = 3; i < length; i++) {

    if (buffer[i] == ':')
      continue;

    if (char_is_unreserved(buffer[i]))
      continue;

    if (char_is_reserved_sub_delimiter(buffer[i]))
      continue;

    return false;
  }

  return true;
}

/*----------------------------------------------------------------------------*/

static bool chars_are_ip_literal(const char *buffer, size_t length) {

  if (!buffer || length < 4)
    return false;

  if (buffer[0] != '[')
    return false;

  if (buffer[length - 1] != ']')
    return false;

  if (chars_are_ip(AF_INET6, buffer + 1, length - 2))
    return true;

  if (chars_are_ip_future(buffer + 1, length - 2))
    return true;

  return false;
}

/*----------------------------------------------------------------------------*/

static bool chars_are_reg_name(const char *buffer, size_t length) {

  if (!buffer)
    return false;

  if (length == 0)
    return true;

  for (size_t i = 0; i < length; i++) {

    if (buffer[i] == '%') {

      if (length < (i + 2))
        return false;

      if (!chars_are_percent_encoded(buffer + i, 3))
        return false;

      i += 2;

    } else {

      if (!char_is_unreserved(buffer[i]))
        if (!char_is_reserved_sub_delimiter(buffer[i]))
          return false;
    }
  }

  return true;
}

/*----------------------------------------------------------------------------*/

static bool chars_are_host(const char *buffer, size_t length) {

  if (!buffer || length < 1)
    return false;

  switch (buffer[length]) {

  case 0:
  case ':':
  case '/':
  case '?':
  case '#':
    break;
  default:
    return false;
  }

  if (chars_are_ip_literal(buffer, length))
    return true;

  if (chars_are_ip(AF_INET, buffer, length))
    return true;

  if (chars_are_reg_name(buffer, length))
    return true;

  return false;
}

/*----------------------------------------------------------------------------*/

static bool chars_are_path_chars(const char *buffer, size_t length,
                                 bool colon) {

  if (!buffer || length < 1)
    return false;

  for (size_t i = 0; i < length; i++) {

    if (buffer[i] == '%') {

      if (!chars_are_percent_encoded(buffer + i, length - i))
        return false;

      i += 2;

      continue;
    }

    switch (buffer[i]) {

    case ':':
      if (!colon) {
        return false;
      } else {
        continue;
      }

    case '@':
      continue;
    }

    if (char_is_unreserved(buffer[i]))
      continue;

    if (char_is_reserved_sub_delimiter(buffer[i]))
      continue;

    return false;
  }

  return true;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #Masking
 *
 *      ------------------------------------------------------------------------
 */

static bool mask_scheme(const char *start, size_t length, char **item,
                        size_t *len, char **next) {

  if (!start || !item || !next || !len)
    goto error;

  char *ptr = memchr(start, ':', length);
  if (!ptr) {

    *item = (char *)start;
    *next = (char *)start;
    *len = 0;
    return true;
  }

  if (!chars_are_scheme(start, (ptr - start))) {

    *item = (char *)start;
    *next = (char *)start;
    *len = 0;
    return true;
  }

  *item = (char *)start;
  *next = ptr + 1;
  *len = ptr - start;

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool mask_user(const char *start, size_t length, char **item,
                      size_t *len, char **next) {

  if (!start || !item || !next || !len)
    goto error;

  char *ptr = memchr(start, '@', length);

  if (!ptr) {

    *item = (char *)start;
    *next = (char *)start;
    *len = 0;

    return true;
  }

  if (!chars_are_user(start, (ptr - start)))
    goto error;

  *item = (char *)start;
  *next = ptr + 1;
  *len = ptr - start;

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static const char *next_general_delimiter(const char *start, size_t length) {

  if (!start)
    goto error;

  for (size_t i = 0; i < length; i++) {

    if (char_is_reserved_general_delimiter(start[i]))
      return start + i;
  }

error:
  return NULL;
}

/*----------------------------------------------------------------------------*/

static const char *next_major_delimiter(const char *start, size_t length) {

  if (!start)
    goto error;

  for (size_t i = 0; i < length; i++) {

    switch (start[i]) {

    case '/':
    case '?':
    case '#':
      return start + i;
    }
  }

error:
  return NULL;
}

/*----------------------------------------------------------------------------*/

static const char *next_ip_future(const char *start, size_t length) {

  if (!start)
    goto error;

  for (size_t i = 0; i < length; i++) {

    if ((start[i] == ':') || (char_is_unreserved(start[i])) ||
        (char_is_reserved_sub_delimiter(start[i])))
      continue;

    return start + i;
  }

error:
  return NULL;
}

/*----------------------------------------------------------------------------*/

static bool mask_host(const char *start, size_t length, char **item,
                      size_t *len, char **next) {

  if (!start || !item || !next || !len)
    goto error;

  if (0 == length) {

    *item = (char *)start;
    *next = (char *)start;
    *len = 0;

    return true;
  }

  *item = NULL;
  *next = NULL;
  *len = 0;

  char *ptr = NULL;

  /*
   * host          = IP-literal / IPv4address / reg-name
   *
   * IP-literal    = "[" ( IPv6address / IPvFuture  ) "]"
   *
   * IPvFuture     = "v" 1*HEXDIG "." 1*( unreserved / sub-delims / ":" )
   *
   * IPv6address   =                            6( h16 ":" ) ls32
   *               /                       "::" 5( h16 ":" ) ls32
   *               / [               h16 ] "::" 4( h16 ":" ) ls32
   *               / [ *1( h16 ":" ) h16 ] "::" 3( h16 ":" ) ls32
   *               / [ *2( h16 ":" ) h16 ] "::" 2( h16 ":" ) ls32
   *               / [ *3( h16 ":" ) h16 ] "::"    h16 ":"   ls32
   *               / [ *4( h16 ":" ) h16 ] "::"              ls32
   *               / [ *5( h16 ":" ) h16 ] "::"              h16
   *               / [ *6( h16 ":" ) h16 ] "::"
   * h16           = 1*4HEXDIG
   * ls32          = ( h16 ":" h16 ) / IPv4address
   * IPv4address   = dec-octet "." dec-octet "." dec-octet "." dec-octet
   * reg-name      = *( unreserved / pct-encoded / sub-delims )
   *
   */

  switch (start[0]) {

  case '[':

    ptr = memchr(start, ']', length);
    if (!ptr)
      goto error;
    ptr++;

    *len = ptr - start;
    *next = ptr;

    if (!chars_are_ip_literal(start, *len))
      goto error;

    break;

  case 'v':

    if (3 < length)
      goto error;
    if (!isxdigit(start[1]))
      goto error;
    if (start[2] != '.')
      goto error;

    ptr = (char *)next_ip_future(start + 2, length - 2);

    if (ptr) {

      *len = ptr - start;
      *next = ptr;

    } else {

      *len = length;
      *next = (char *)start + length;
    }

    if (!chars_are_ip_future((char *)start, *len))
      goto error;

    break;

  default:

    ptr = (char *)next_general_delimiter(start, length);

    if (ptr) {

      *len = ptr - start;
      *next = ptr;

    } else {

      *len = length;
      *next = (char *)start + length;
    }

    if ((!chars_are_ip(AF_INET, start, *len)) &&
        (!chars_are_reg_name(start, *len)))
      goto error;

    break;
  }

  *item = (char *)start;
  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool mask_port(const char *start, size_t length, char **item,
                      size_t *len, char **next, ov_uri *uri) {

  if (!start || !item || !next)
    goto error;

  /*
   *      no port string contained?
   */

  if (0 == length) {

    *item = (char *)start;
    *next = (char *)start;
    *len = 0;

    return true;
  }

  if (start[0] != ':') {

    *item = (char *)start;
    *next = (char *)start;
    *len = 0;

    return true;
  }

  /*
   *      check port string
   */

  if (length < 2)
    goto error;

  int64_t number = 0;
  number = strtoll(start + 1, next, 10);

  if (number < 0)
    goto error;
  if (number > 65535)
    goto error;

  *item = (char *)start + 1;

  if (*next) {
    *len = (*next - *item);
  } else {
    *len = length - 1;
  }

  if (uri)
    uri->port = number;

  return true;
error:
  return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #checks
 *
 *      ------------------------------------------------------------------------
 */

/*----------------------------------------------------------------------------*/

static bool check_with_delimiter(const char *start, size_t length, char **next,
                                 char delimiter, bool colon) {

  if (!start || !next || !delimiter)
    goto error;

  char *ptr = (char *)start;
  char *nxt = NULL;

  /*
   *      Verify all segments up to last delimiter.
   */

  do {
    nxt = memchr(ptr, delimiter, length - (ptr - start));

    if (nxt) {

      if (!chars_are_path_chars(ptr, (nxt - ptr), colon))
        goto error;

      ptr = nxt + 1;
    }

    colon = true;

  } while (nxt);

  /*
   *      Verify last segment/item
   */

  nxt = (char *)next_major_delimiter(ptr, length - (ptr - start));

  if (!nxt) {

    if ((ssize_t)length > (ptr - start))
      if (!chars_are_path_chars(ptr, length - (ptr - start), colon))
        goto error;

    *next = (char *)start + length;

  } else {

    if (nxt != ptr)
      if (!chars_are_path_chars(ptr, nxt - ptr, colon))
        goto error;

    *next = nxt;
  }

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool check_path(const char *start, size_t length, char **next,
                       bool colon) {

  return check_with_delimiter(start, length, next, '/', colon);
}

/*----------------------------------------------------------------------------*/

static bool check_path_absolute(const char *start, size_t length, char **next,
                                ov_uri *uri) {

  if (!start || !next || length < 1)
    goto error;

  /* path-absolute   ; begins with "/" but not "//" */

  if (start[0] != '/')
    goto error;
  if (start[1] == '/')
    goto error;

  if (!check_path(start + 1, length - 1, next, true))
    goto error;

  if (uri)
    uri->path = strndup(start, (*next - start));

  return true;

error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool check_with_authority(const char *start, size_t length, char **next,
                                 ov_uri *uri) {

  if (!start || !next || length < 2)
    goto error;

  size_t len = 0;
  char *ptr = NULL;
  char *nxt = (char *)start;

  if (!mask_user(nxt, length - (nxt - start), &ptr, &len, &nxt))
    goto error;

  if (uri && (len > 0))
    uri->user = strndup(ptr, len);

  if (!mask_host(nxt, length - (nxt - start), &ptr, &len, &nxt))
    goto error;

  if (uri && len > 0)
    uri->host = strndup(ptr, len);

  if (!mask_port(nxt, length - (nxt - start), &ptr, &len, &nxt, uri))
    goto error;

  *next = nxt;

  if ((ssize_t)length == (nxt - start))
    return true;

  switch (nxt[0]) {

  case '?':
  case '#':
    /* path empty */
    return true;
  }

  return check_path_absolute(nxt, length - (nxt - start), next, uri);

error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool check_path_rootless(const char *start, size_t length, char **next,
                                ov_uri *uri) {

  if (!start || !next || length < 1)
    goto error;

  /* path-rootless   ; begins with a segment */

  if (start[0] == '/')
    goto error;

  if (check_with_authority(start, length, next, uri))
    return true;

  if (!check_path(start, length, next, true))
    goto error;

  if (uri)
    uri->path = strndup(start, (*next - start));

  return true;

error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool check_path_no_scheme(const char *start, size_t length, char **next,
                                 ov_uri *uri) {

  if (!start || !next || length < 1)
    goto error;

  /* path-noscheme   ; begins with a non-colon segment */

  if (start[0] == '/')
    goto error;
  if (!check_path(start, length, next, false))
    goto error;

  if (uri)
    uri->path = strndup(start, (*next - start));

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool check_path_with_authority(const char *start, size_t length,
                                      char **next, ov_uri *uri) {

  if (!start || !next || length < 2)
    goto error;

  if (start[0] != '/')
    goto error;
  if (start[1] != '/')
    goto error;

  size_t len = 0;
  char *ptr = NULL;
  char *nxt = (char *)start + 2;

  if (!mask_user(nxt, length - (nxt - start), &ptr, &len, &nxt))
    goto error;

  if (uri && (len > 0))
    uri->user = strndup(ptr, len);

  if (!mask_host(nxt, length - (nxt - start), &ptr, &len, &nxt))
    goto error;

  if (uri && len > 0)
    uri->host = strndup(ptr, len);

  if (!mask_port(nxt, length - (nxt - start), &ptr, &len, &nxt, uri))
    goto error;

  *next = nxt;

  if ((ssize_t)length == (nxt - start))
    return true;

  switch (nxt[0]) {

  case '?':
  case '#':
    /* path empty */
    return true;
  }

  return check_path_absolute(nxt, length - (nxt - start), next, uri);

error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool check_hierarchical_path(const char *start, size_t length,
                                    char **next, ov_uri *uri) {

  if (!start || !next)
    goto error;

  /*
   *      hier-part     = "//" authority path-abempty
   *                      / path-absolute
   *                      / path-rootless
   *                      / path-empty
   *
   */

  if (0 == length)
    return true;

  switch (start[0]) {

  case '/':

    if (check_path_absolute(start, length, next, uri))
      return true;

    return check_path_with_authority(start, length, next, uri);

  case '?':
    return true;

  default:
    return check_path_rootless(start, length, next, uri);
  }
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool check_relative_path(const char *start, size_t length, char **next,
                                ov_uri *uri) {

  if (!start || !next)
    goto error;

  /*
   *      relative-part = "//" authority path-abempty
   *                      / path-absolute
   *                      / path-noscheme
   *                      / path-empty
   *
   */

  if (0 == length)
    return true;

  switch (start[0]) {

  case '/':

    if (check_path_absolute(start, length, next, uri))
      return true;

    return check_path_with_authority(start, length, next, uri);

  case '?':
  case '#':
    return true;

  default:
    return check_path_no_scheme(start, length, next, uri);
  }
error:
  return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #PUBLIC
 *
 *      ------------------------------------------------------------------------
 */

bool ov_uri_clear(ov_uri *uri) {

  if (!uri)
    return false;

  uri->scheme = ov_data_pointer_free(uri->scheme);
  uri->path = ov_data_pointer_free(uri->path);
  uri->user = ov_data_pointer_free(uri->user);
  uri->host = ov_data_pointer_free(uri->host);
  uri->query = ov_data_pointer_free(uri->query);
  uri->fragment = ov_data_pointer_free(uri->fragment);
  uri->scheme = ov_data_pointer_free(uri->scheme);
  uri->port = 0;

  return true;
}

/*----------------------------------------------------------------------------*/

ov_uri *ov_uri_free(ov_uri *uri) {

  if (!ov_uri_clear(uri))
    return uri;
  free(uri);
  return NULL;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      VALIDATION FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

bool ov_uri_string_is_valid(const char *string, size_t length) {

  if (ov_uri_string_is_absolute(string, length))
    return true;

  return ov_uri_string_is_reference(string, length);
}

/*----------------------------------------------------------------------------*/

static bool parse_uri_string_absolute(const char *start, size_t length,
                                      ov_uri *uri) {

  if (!start || (0 == length))
    goto error;

  /* absolute-URI  = scheme ":" hier-part [ "?" query ] */

  if (!chars_are_allowed(start, length))
    goto error;

  char *ptr = NULL;
  char *next = NULL;
  size_t len = 0;

  if (!mask_scheme(start, length, &ptr, &len, &next))
    goto error;

  if (0 == len)
    goto error;

  if (uri)
    uri->scheme = strndup(ptr, len);

  if (!check_hierarchical_path(next, (length - (next - start)), &next, uri))
    goto error;

  if ((ssize_t)length == next - start)
    return true;

  if (*next != '?')
    goto error;

  char *query = next;
  next++;

  if (!check_with_delimiter(next, length - (next - start), &next, '?', true))
    goto error;

  if (uri)
    uri->query = strndup(query, next - query);

  if ((ssize_t)length == next - start)
    return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

bool ov_uri_string_is_absolute(const char *start, size_t length) {
  return parse_uri_string_absolute(start, length, NULL);
}

/*----------------------------------------------------------------------------*/

static bool parse_uri_string_reference(const char *start, size_t length,
                                       ov_uri *uri) {

  if (!start || (0 == length))
    goto error;

  if (!chars_are_allowed(start, length))
    goto error;

  char *ptr = NULL;
  char *next = NULL;
  size_t len = 0;

  if (!mask_scheme(start, length, &ptr, &len, &next))
    goto error;

  if (uri && len > 0)
    uri->scheme = strndup(ptr, len);

  if (!check_relative_path(next, (length - (next - start)), &next, uri))
    goto error;

  if ((ssize_t)length == next - start)
    return true;

  if (*next == '?') {

    char *query = next;

    next++;
    if (!check_with_delimiter(next, length - (next - start), &next, '?', true))
      goto error;

    if (uri)
      uri->query = strndup(query, next - query);

    if ((ssize_t)length == next - start)
      return true;
  }

  if (*next != '#')
    goto error;

  char *fragment = next;

  next++;

  if (!check_with_delimiter(next, length - (next - start), &next, '#', true))
    goto error;

  if (uri)
    uri->fragment = strndup(fragment, next - fragment);

  if ((ssize_t)length == next - start)
    return true;

error:
  return false;
}

/*----------------------------------------------------------------------------*/

bool ov_uri_string_is_reference(const char *start, size_t length) {
  return parse_uri_string_reference(start, length, NULL);
}

/*----------------------------------------------------------------------------*/

/*
 *      ------------------------------------------------------------------------
 *
 *      DE / ENCODING (copy based)
 *
 *      ------------------------------------------------------------------------
 */

ov_uri *ov_uri_from_string(const char *string, size_t length) {

  ov_uri *uri = NULL;
  if (!string || (0 == length))
    goto error;

  uri = calloc(1, sizeof(ov_uri));
  if (parse_uri_string_absolute(string, length, uri))
    return uri;

  ov_uri_clear(uri);

  if (parse_uri_string_reference(string, length, uri))
    return uri;
error:
  uri = ov_uri_free(uri);
  return NULL;
}

/*----------------------------------------------------------------------------*/

size_t ov_uri_string_length(const ov_uri *uri) {

  if (!uri)
    goto error;

  size_t len = 0;
  size_t length = 0;

  char *next = NULL;

  if (uri->scheme) {

    len = strlen(uri->scheme);

    if (!chars_are_scheme(uri->scheme, len))
      goto error;

    length += len + 1;
  }

  if (uri->host || uri->user || (0 != uri->port))
    length += 2;

  if (uri->user) {

    len = strlen(uri->user);

    if (!chars_are_user(uri->user, len))
      goto error;

    length += len + 1;
  }

  if (uri->host) {

    len = strlen(uri->host);

    if (!chars_are_host(uri->host, len))
      goto error;

    length += len;
  }

  if (0 != uri->port)
    length += 6;

  if (uri->path) {

    len = strlen(uri->path);
    length += len;
  }

  if (uri->query) {

    if (uri->query[0] != '?')
      goto error;
    len = strlen(uri->query);

    if (!check_with_delimiter(uri->query + 1, len - 1, &next, '?', true))
      goto error;

    length += len;
  }

  if (uri->fragment) {

    if (uri->fragment[0] != '#')
      goto error;
    len = strlen(uri->fragment);

    if (!check_with_delimiter(uri->fragment + 1, len - 1, &next, '#', true))
      goto error;

    length += len;
  }

  return length + 1;
error:
  return 0;
}

/*----------------------------------------------------------------------------*/

char *ov_uri_to_string(const ov_uri *uri) {

  char *out = NULL;
  if (!uri)
    goto error;

  size_t len = 0;
  size_t length = ov_uri_string_length(uri);
  if (0 == length)
    goto error;

  out = calloc(length, sizeof(char));
  if (!out)
    goto error;

  char *ptr = out;

  if (uri->scheme) {

    len = snprintf(ptr, length - (ptr - out), "%s:", uri->scheme);

    if (len <= 0)
      goto error;
    ptr += len;
  }

  if (uri->user || uri->host || (uri->port > 0)) {

    len = snprintf(ptr, length - (ptr - out), "//");

    if (len <= 0)
      goto error;
    ptr += len;
  }

  if (uri->user) {

    len = snprintf(ptr, length - (ptr - out), "%s@", uri->user);

    if (len <= 0)
      goto error;
    ptr += len;
  }

  if (uri->host) {

    len = snprintf(ptr, length - (ptr - out), "%s", uri->host);

    if (len <= 0)
      goto error;
    ptr += len;
  }

  if (0 != uri->port) {

    len = snprintf(ptr, length - (ptr - out), ":%i", uri->port);

    if (len <= 0)
      goto error;
    ptr += len;
  }

  if (uri->path) {

    len = snprintf(ptr, length - (ptr - out), "%s", uri->path);

    if (len <= 0)
      goto error;
    ptr += len;
  }

  if (uri->query) {

    len = snprintf(ptr, length - (ptr - out), "%s", uri->query);

    if (len <= 0)
      goto error;
    ptr += len;
  }

  if (uri->fragment) {

    len = snprintf(ptr, length - (ptr - out), "%s", uri->fragment);

    if (len <= 0)
      goto error;
    ptr += len;
  }

  return out;
error:
  out = ov_data_pointer_free(out);
  return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_uri_path_remove_dot_segments(const char *in, char *out) {

  if (!in || !out)
    goto error;

  size_t len = 0;
  size_t size = strlen(in);
  if (size == 0)
    goto error;

  ov_memory_pointer curr = {0};

  char *wptr = (char *)out;
  char *ptr = (char *)in;
  char *del = memchr(in, '/', size - (ptr - in));

  if (del == ptr) {
    *wptr = '/';
    wptr++;
    ptr++;
    del = memchr(ptr, '/', size - (ptr - in));
  }

  size_t level = 0;

  while (del) {

    /* Check current segment */

    curr.start = (uint8_t *)ptr;
    curr.length = del - ptr;

    /* Ignore empty path e.g. // */

    if (curr.length == 0) {
      ptr = del + 1;
      del = memchr(ptr, '/', size - (ptr - in));
      continue;
    }

    switch (curr.length) {

    case 1:

      /* Ignore ./ */

      if (curr.start[0] == '.') {
        ptr = del + 1;
        del = memchr(ptr, '/', size - (ptr - in));
        continue;
      }

      break;

    case 2:

      if ((curr.start[0] == '.') && (curr.start[1] == '.')) {

        if (level == 0)
          goto error;

        /* remove prev */

        len = wptr - out;
        size_t i = len;

        for (i = len; i > 0; i--) {

          if (out[i] == '/') {
            out[i] = 0;
            wptr = out + i - 1;
            break;
          }
        }

        if (i == 0)
          wptr = out;

        level--;

        ptr = del + 1;
        del = memchr(ptr, '/', size - (ptr - in));
        continue;
      }

      break;

    default:
      break;
    }

    len = del - ptr;

    if (!strncpy(wptr, (char *)ptr, len))
      goto error;

    wptr += len;

    /* add delimiter to out */

    *wptr = '/';
    wptr++;

    level++;

    ptr = del + 1;
    del = memchr(ptr, '/', size - (ptr - in));
  }

  /* add last to out e.g. index.html */

  if (!strncpy(wptr, ptr, size - (ptr - in)))
    goto error;

  // fprintf(stdout, "IN : %s\nOUT: %s\n", in, out);
  return true;
error:
  if (out)
    memset(out, 0, PATH_MAX);
  return false;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_uri_to_json(const ov_uri *uri) {

  ov_json_value *out = NULL;
  ov_json_value *val = NULL;

  if (!uri)
    goto error;

  out = ov_json_object();

  if (NULL != uri->scheme) {

    val = ov_json_string(uri->scheme);
    if (!ov_json_object_set(out, OV_KEY_SCHEME, val))
      goto error;
  }

  if (NULL != uri->path) {

    val = ov_json_string(uri->path);
    if (!ov_json_object_set(out, OV_KEY_PATH, val))
      goto error;
  }

  if (NULL != uri->user) {

    val = ov_json_string(uri->user);
    if (!ov_json_object_set(out, OV_KEY_USER, val))
      goto error;
  }

  if (NULL != uri->host) {

    val = ov_json_string(uri->host);
    if (!ov_json_object_set(out, OV_KEY_HOST, val))
      goto error;
  }

  if (0 != uri->port) {

    val = ov_json_number(uri->port);
    if (!ov_json_object_set(out, OV_KEY_PORT, val))
      goto error;
  }

  if (NULL != uri->query) {

    val = ov_json_string(uri->query);
    if (!ov_json_object_set(out, OV_KEY_QUERY, val))
      goto error;
  }

  if (NULL != uri->fragment) {

    val = ov_json_string(uri->fragment);
    if (!ov_json_object_set(out, OV_KEY_FRAGMENT, val))
      goto error;
  }

  return out;
error:
  ov_json_value_free(out);
  ov_json_value_free(val);
  return NULL;
}
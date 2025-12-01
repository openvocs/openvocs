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
        @file           ov_uri_test.c
        @author         Markus Töpfer

        @date           2020-01-03

        @ingroup        ov_uri

        @brief          Unit tests of


        ------------------------------------------------------------------------
*/
#include "ov_uri.c"
#include <ov_test/testrun.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_uri_string_is_valid() {

  char *string = "http://www.openvocs.org/api?user?auth#password";
  size_t len = strlen(string);

  testrun(!ov_uri_string_is_valid(NULL, 0));
  testrun(!ov_uri_string_is_valid(string, 0));
  testrun(!ov_uri_string_is_valid(NULL, len));

  testrun(ov_uri_string_is_valid(string, len));

  string = "http://openvocs.org:1234/home?query#fragment";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "http://user@openvocs.org/home?query#fragment";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "http://user@openvocs.org:1234?query#fragment";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "http://user@openvocs.org:1234/?query";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "http://user@openvocs.org:1234/#fragment";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "http://user@openvocs.org:1234/home#fragment";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "/home?query#fragment";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "?query#fragment";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "#fragment";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "xml://user@openvocs.org:1234/home?query#fragment";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "xml";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "//user:name@host/path";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "scheme:#frag";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "scheme:?query";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "scheme:path";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "http://de.wikipedia.org/wiki/URI";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "file:///etc/fstab";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "file:///C:/Users%20Res%20Id.html";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "geo:48.33,14.122;u=22.5";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "ldap://[2001:db8::7]/c=GB?objectClass?one";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "gopher://gopher.floodgap.com";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "mailto:John.Doe@example.com";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "sip:911@pbx.mycompany.com";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "news:comp.infosystems.www.servers.unix";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "data:text/plain;charset=iso-8859-7,%be%fa%be";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "tel:+1-816-555-1212";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "telnet://192.0.2.16:80/";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "urn:oasis:names:specification:docbook:dtd:xml:4.1.2";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "crid://broadcaster.com/movies/BestActionMovieEver";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "http://nobody:password@example.org:8080/cgi-bin/"
           "script.php?action=submit&pageid=86392001#section_2";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "http://user@host:1234/path?query#fragment";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "//user@host:1234/path?query#fragment";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "path?query#fragment";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "x:";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "path";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "//user@host";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "?query";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "#fragment";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "scheme:path";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "scheme:path#frag";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "scheme:path?query";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "scheme:path?query#frag";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "scheme:/path";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "scheme:/path#frag";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "scheme:/path?query";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "scheme:/path?query#frag";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "scheme:#frag";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "scheme:?query";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "scheme:?query#frag";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "scheme://user@host/path";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "scheme://user@host/path#frag";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "scheme://user@host/path?query";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "scheme://user@host/path?query#frag";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "http://de.wikipedia.org/wiki/URI";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "file:///C:/Users%20Res%20Id.html";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "file:///etc/fstab";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "geo:48.33,14.122;u=22.5";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "ldap://[2001:db8::7]/c=GB?objectClass?one";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "gopher://gopher.floodgap.com";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "news:comp.infosystems.www.servers.unix";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "data:text/plain;charset=iso-8859-7,%be%fa%be";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "tel:+1-816-555-1212";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "telnet://192.0.2.16:80/";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "urn:oasis:names:specification:docbook:dtd:xml:4.1.2";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "crid://broadcaster.com/movies/BestActionMovieEver";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "http://nobody:password@example.org:8080/cgi-bin/"
           "script.php?action=submit&pageid=86392001#section_2";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  string = "http://nobody:password@example.org:8080/cgi-bin/"
           "script.php?action=submit&pageid=86392001#section_2";
  testrun(ov_uri_string_is_valid(string, strlen(string)));

  // negative tests

  string = "user@host:1234/path?query#fragment";
  testrun(!ov_uri_string_is_valid(string, strlen(string)));

  string = "scheme:error\%path";
  testrun(!ov_uri_string_is_valid(string, strlen(string)));

  // TBD

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_uri_from_string() {

  char *string = "http://www.openvocs.org/api?user?auth#password";
  size_t len = strlen(string);

  testrun(!ov_uri_from_string(NULL, 0));
  testrun(!ov_uri_from_string(string, 0));
  testrun(!ov_uri_from_string(NULL, len));

  ov_uri *uri = ov_uri_from_string(string, len);
  testrun(uri);
  testrun(0 == strncmp(uri->scheme, "http", 4));
  testrun(0 == strncmp(uri->host, "www.openvocs.org", 16));
  testrun(0 == strncmp(uri->path, "/api", 4));
  testrun(0 == strncmp(uri->query, "?user?auth", 10));
  testrun(0 == strncmp(uri->fragment, "#password", 9));
  testrun(0 == uri->user);
  testrun(0 == uri->port);
  uri = ov_uri_free(uri);

  string = "http://openvocs.org:1234/home?query#fragment";
  uri = ov_uri_from_string(string, strlen(string));
  testrun(uri);
  testrun(0 == strncmp(uri->scheme, "http", 4));
  testrun(0 == strncmp(uri->host, "openvocs.org", 12));
  testrun(0 == strncmp(uri->path, "/home", 5));
  testrun(0 == strncmp(uri->query, "?query", 6));
  testrun(0 == strncmp(uri->fragment, "#fragment", 9));
  testrun(0 == uri->user);
  testrun(1234 == uri->port);
  uri = ov_uri_free(uri);

  string = "sip:loop1@168.129.1.1";
  uri = ov_uri_from_string(string, strlen(string));
  testrun(uri);
  testrun(0 == strncmp(uri->scheme, "sip", 3));
  testrun(0 == uri->path);
  testrun(0 == uri->query);
  testrun(0 == uri->fragment);
  testrun(0 == uri->port);
  testrun(0 != uri->host);
  testrun(0 != uri->user);
  testrun(0 == strncmp(uri->host, "168.129.1.1", 11));
  testrun(0 == strncmp(uri->user, "loop1", 5));

  string = "mailto:user@inbox";
  uri = ov_uri_from_string(string, strlen(string));
  testrun(uri);
  testrun(0 == strncmp(uri->scheme, "mailto", 5));
  testrun(0 == uri->path);
  testrun(0 == uri->query);
  testrun(0 == uri->fragment);
  testrun(0 == uri->port);
  testrun(0 != uri->host);
  testrun(0 != uri->user);
  testrun(0 == strncmp(uri->host, "inbox", 5));
  testrun(0 == strncmp(uri->user, "user", 4));

  uri = ov_uri_free(uri);

  string = "http://user@openvocs.org/home?query#fragment";
  uri = ov_uri_from_string(string, strlen(string));
  testrun(uri);
  testrun(0 == strncmp(uri->scheme, "http", 4));
  testrun(0 == strncmp(uri->host, "openvocs.org", 12));
  testrun(0 == strncmp(uri->path, "/home", 5));
  testrun(0 == strncmp(uri->query, "?query", 6));
  testrun(0 == strncmp(uri->fragment, "#fragment", 9));
  testrun(0 == strncmp(uri->user, "user", 4));
  testrun(0 == uri->port);
  uri = ov_uri_free(uri);

  string = "http://user@openvocs.org:1234?query#fragment";
  uri = ov_uri_from_string(string, strlen(string));
  testrun(uri);
  testrun(0 == strncmp(uri->scheme, "http", 4));
  testrun(0 == strncmp(uri->host, "openvocs.org", 12));
  testrun(0 == uri->path);
  testrun(0 == strncmp(uri->query, "?query", 6));
  testrun(0 == strncmp(uri->fragment, "#fragment", 9));
  testrun(0 == strncmp(uri->user, "user", 4));
  testrun(1234 == uri->port);
  uri = ov_uri_free(uri);

  string = "http://user@openvocs.org:1234/?query";
  uri = ov_uri_from_string(string, strlen(string));
  testrun(uri);
  testrun(0 == strncmp(uri->scheme, "http", 4));
  testrun(0 == strncmp(uri->host, "openvocs.org", 12));
  testrun(0 == strncmp(uri->path, "/", 1));
  testrun(0 == strncmp(uri->query, "?query", 6));
  testrun(0 == uri->fragment);
  testrun(0 == strncmp(uri->user, "user", 4));
  testrun(1234 == uri->port);
  uri = ov_uri_free(uri);

  string = "http://user@openvocs.org:1234/#fragment";
  uri = ov_uri_from_string(string, strlen(string));
  testrun(uri);
  testrun(0 == strncmp(uri->scheme, "http", 4));
  testrun(0 == strncmp(uri->host, "openvocs.org", 12));
  testrun(0 == strncmp(uri->path, "/", 1));
  testrun(0 == uri->query);
  testrun(0 == strncmp(uri->fragment, "#fragment", 9));
  testrun(0 == strncmp(uri->user, "user", 4));
  testrun(1234 == uri->port);
  uri = ov_uri_free(uri);

  string = "http://user@openvocs.org:1234/home#fragment";
  uri = ov_uri_from_string(string, strlen(string));
  testrun(uri);
  testrun(0 == strncmp(uri->scheme, "http", 4));
  testrun(0 == strncmp(uri->host, "openvocs.org", 12));
  testrun(0 == strncmp(uri->path, "/home", 5));
  testrun(0 == uri->query);
  testrun(0 == strncmp(uri->fragment, "#fragment", 9));
  testrun(0 == strncmp(uri->user, "user", 4));
  testrun(1234 == uri->port);
  uri = ov_uri_free(uri);

  string = "/home?query#fragment";
  uri = ov_uri_from_string(string, strlen(string));
  testrun(uri);
  testrun(0 == uri->scheme);
  testrun(0 == uri->host);
  testrun(0 == uri->user);
  testrun(0 == uri->port);
  testrun(0 == strncmp(uri->path, "/home", 4));
  testrun(0 == strncmp(uri->query, "?query", 6));
  testrun(0 == strncmp(uri->fragment, "#fragment", 9));
  uri = ov_uri_free(uri);

  string = "?query#fragment";
  uri = ov_uri_from_string(string, strlen(string));
  testrun(uri);
  testrun(0 == uri->scheme);
  testrun(0 == uri->host);
  testrun(0 == uri->user);
  testrun(0 == uri->port);
  testrun(0 == uri->path);
  testrun(0 == strncmp(uri->query, "?query", 6));
  testrun(0 == strncmp(uri->fragment, "#fragment", 9));
  uri = ov_uri_free(uri);

  string = "#fragment";
  uri = ov_uri_from_string(string, strlen(string));
  testrun(uri);
  testrun(0 == uri->scheme);
  testrun(0 == uri->host);
  testrun(0 == uri->user);
  testrun(0 == uri->port);
  testrun(0 == uri->path);
  testrun(0 == uri->query);
  testrun(0 == strncmp(uri->fragment, "#fragment", 9));
  uri = ov_uri_free(uri);

  string = "xml";
  uri = ov_uri_from_string(string, strlen(string));
  testrun(uri);
  testrun(0 == uri->scheme);
  testrun(0 == uri->host);
  testrun(0 == uri->user);
  testrun(0 == uri->port);
  testrun(0 == strncmp(uri->path, "xml", 3));
  testrun(0 == uri->query);
  testrun(0 == uri->fragment);
  uri = ov_uri_free(uri);

  string = "//user:name@host/path";
  uri = ov_uri_from_string(string, strlen(string));
  testrun(uri);
  testrun(0 == uri->scheme);
  testrun(0 == strncmp(uri->host, "host", 4));
  testrun(0 == strncmp(uri->path, "/path", 5));
  testrun(0 == uri->query);
  testrun(0 == uri->fragment);
  testrun(0 == strncmp(uri->user, "user:name", 9));
  testrun(0 == uri->port);
  uri = ov_uri_free(uri);

  string = "file:///etc/fstab";
  uri = ov_uri_from_string(string, strlen(string));
  testrun(uri);
  testrun(0 == strncmp(uri->scheme, "file", 4));
  testrun(0 == uri->host);
  testrun(0 == uri->user);
  testrun(0 == uri->port);
  testrun(0 == strncmp(uri->path, "/etc/fstab", 10));
  testrun(0 == uri->query);
  testrun(0 == uri->fragment);
  uri = ov_uri_free(uri);

  // negative tests

  string = "user@host:1234/path?query#fragment";
  testrun(!ov_uri_from_string(string, strlen(string)));

  string = "scheme:error\%path";
  testrun(!ov_uri_from_string(string, strlen(string)));

  // TBD

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_uri_to_string() {

  char *string = "http://www.openvocs.org/api?user?auth#password";
  size_t len = strlen(string);
  char *out = NULL;
  ov_uri *uri = NULL;

  testrun(!ov_uri_to_string(NULL));

  uri = ov_uri_from_string(string, len);
  testrun(uri);
  out = ov_uri_to_string(uri);
  testrun(out);
  testrun(0 == strncmp(out, string, strlen(string)));
  out = ov_data_pointer_free(out);
  uri = ov_uri_free(uri);

  string = "http://openvocs.org:1234/home?query#fragment";
  uri = ov_uri_from_string(string, strlen(string));
  testrun(uri);
  out = ov_uri_to_string(uri);
  testrun(out);
  testrun(0 == strncmp(out, string, strlen(string)));
  out = ov_data_pointer_free(out);
  uri = ov_uri_free(uri);

  string = "http://user@openvocs.org/home?query#fragment";
  uri = ov_uri_from_string(string, strlen(string));
  testrun(uri);
  out = ov_uri_to_string(uri);
  testrun(out);
  testrun(0 == strncmp(out, string, strlen(string)));
  out = ov_data_pointer_free(out);
  uri = ov_uri_free(uri);

  string = "http://user@openvocs.org:1234?query#fragment";
  uri = ov_uri_from_string(string, strlen(string));
  testrun(uri);
  out = ov_uri_to_string(uri);
  testrun(out);
  testrun(0 == strncmp(out, string, strlen(string)));
  out = ov_data_pointer_free(out);
  uri = ov_uri_free(uri);

  string = "http://user@openvocs.org:1234/?query";
  uri = ov_uri_from_string(string, strlen(string));
  testrun(uri);
  out = ov_uri_to_string(uri);
  testrun(out);
  testrun(0 == strncmp(out, string, strlen(string)));
  out = ov_data_pointer_free(out);
  uri = ov_uri_free(uri);

  string = "http://user@openvocs.org:1234/#fragment";
  uri = ov_uri_from_string(string, strlen(string));
  testrun(uri);
  out = ov_uri_to_string(uri);
  testrun(out);
  testrun(0 == strncmp(out, string, strlen(string)));
  out = ov_data_pointer_free(out);
  uri = ov_uri_free(uri);

  string = "http://user@openvocs.org:1234/home#fragment";
  uri = ov_uri_from_string(string, strlen(string));
  testrun(uri);
  out = ov_uri_to_string(uri);
  testrun(out);
  testrun(0 == strncmp(out, string, strlen(string)));
  out = ov_data_pointer_free(out);
  uri = ov_uri_free(uri);

  string = "/home?query#fragment";
  uri = ov_uri_from_string(string, strlen(string));
  testrun(uri);
  out = ov_uri_to_string(uri);
  testrun(out);
  testrun(0 == strncmp(out, string, strlen(string)));
  out = ov_data_pointer_free(out);
  uri = ov_uri_free(uri);

  string = "?query#fragment";
  uri = ov_uri_from_string(string, strlen(string));
  testrun(uri);
  out = ov_uri_to_string(uri);
  testrun(out);
  testrun(0 == strncmp(out, string, strlen(string)));
  out = ov_data_pointer_free(out);
  uri = ov_uri_free(uri);

  string = "#fragment";
  uri = ov_uri_from_string(string, strlen(string));
  testrun(uri);
  out = ov_uri_to_string(uri);
  testrun(out);
  testrun(0 == strncmp(out, string, strlen(string)));
  out = ov_data_pointer_free(out);
  uri = ov_uri_free(uri);

  string = "xml";
  uri = ov_uri_from_string(string, strlen(string));
  testrun(uri);
  out = ov_uri_to_string(uri);
  testrun(out);
  testrun(0 == strncmp(out, string, strlen(string)));
  out = ov_data_pointer_free(out);
  uri = ov_uri_free(uri);

  string = "//user:name@host/path";
  uri = ov_uri_from_string(string, strlen(string));
  testrun(uri);
  out = ov_uri_to_string(uri);
  testrun(out);
  testrun(0 == strncmp(out, string, strlen(string)));
  out = ov_data_pointer_free(out);
  uri = ov_uri_free(uri);

  string = "file://x/etc/fstab";
  uri = ov_uri_from_string(string, strlen(string));
  testrun(uri);
  out = ov_uri_to_string(uri);
  testrun(out);
  testrun(0 == strncmp(out, string, strlen(string)));
  out = ov_data_pointer_free(out);
  uri = ov_uri_free(uri);

  char *expect = "file:/etc/fstab";
  string = "file:///etc/fstab";
  uri = ov_uri_from_string(string, strlen(string));
  testrun(uri);
  out = ov_uri_to_string(uri);
  testrun(out);
  testrun(0 != strncmp(out, string, strlen(string)));
  testrun(0 == strncmp(out, expect, strlen(expect)));
  out = ov_data_pointer_free(out);

  testrun(ov_uri_clear(uri));
  expect = "";
  out = ov_uri_to_string(uri);
  testrun(out);
  testrun(0 == strncmp(out, expect, strlen(expect)));
  out = ov_data_pointer_free(out);
  uri = ov_uri_free(uri);

  // negative tests
  // TBD

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_uri_string_is_absolute() {

  char *string = "http://www.openvocs.org/api?user?auth";
  size_t len = strlen(string);

  testrun(!ov_uri_string_is_absolute(NULL, 0));
  testrun(!ov_uri_string_is_absolute(string, 0));
  testrun(!ov_uri_string_is_absolute(NULL, len));

  testrun(ov_uri_string_is_absolute(string, strlen(string)));

  string = "xml:/api?user";
  testrun(ov_uri_string_is_absolute(string, strlen(string)));

  string = "xml:path";
  testrun(ov_uri_string_is_absolute(string, strlen(string)));

  string = "xml:?query";
  testrun(ov_uri_string_is_absolute(string, strlen(string)));

  string = "path";
  testrun(!ov_uri_string_is_absolute(string, strlen(string)));

  string = "http://www.openvocs.org/api?user?auth#fragment";
  testrun(!ov_uri_string_is_absolute(string, strlen(string)));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_uri_string_is_reference() {

  char *string = "http://www.openvocs.org/api?user?auth#password";
  size_t len = strlen(string);

  testrun(!ov_uri_string_is_reference(NULL, 0));
  testrun(!ov_uri_string_is_reference(string, 0));
  testrun(!ov_uri_string_is_reference(NULL, len));

  testrun(ov_uri_string_is_reference(string, strlen(string)));

  string = "xml:/api?user?auth#password";
  testrun(ov_uri_string_is_reference(string, strlen(string)));

  string = "xml:path";
  testrun(ov_uri_string_is_reference(string, strlen(string)));

  string = "path";
  testrun(ov_uri_string_is_reference(string, strlen(string)));

  string = "scheme:colon:path/scheme";
  testrun(!ov_uri_string_is_reference(string, strlen(string)));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_uri_string_length() {

  char *string = "http://www.openvocs.org/api?user?auth#password";
  size_t len = strlen(string);

  testrun(!ov_uri_string_is_reference(NULL, 0));
  testrun(!ov_uri_string_is_reference(string, 0));
  testrun(!ov_uri_string_is_reference(NULL, len));

  testrun(ov_uri_string_is_reference(string, strlen(string)));

  string = "xml:/api?user?auth#password";
  testrun(ov_uri_string_is_reference(string, strlen(string)));

  string = "xml:path";
  testrun(ov_uri_string_is_reference(string, strlen(string)));

  string = "path";
  testrun(ov_uri_string_is_reference(string, strlen(string)));

  string = "scheme:colon:path/scheme";
  testrun(!ov_uri_string_is_reference(string, strlen(string)));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_uri_clear() {

  char *string = "http://user@openvocs.org:1234/api?user?auth#password";

  ov_uri *uri = ov_uri_from_string(string, strlen(string));
  testrun(uri);
  testrun(!ov_uri_clear(NULL));

  testrun(NULL != uri->scheme);
  testrun(NULL != uri->user);
  testrun(NULL != uri->host);
  testrun(0 != uri->port);

  testrun(NULL != uri->query);
  testrun(NULL != uri->fragment);

  testrun(ov_uri_clear(uri));

  testrun(NULL == uri->scheme);
  testrun(NULL == uri->user);
  testrun(NULL == uri->host);
  testrun(0 == uri->port);

  testrun(NULL == uri->query);
  testrun(NULL == uri->fragment);

  testrun(NULL == ov_uri_free(uri));
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_uri_free() {

  // tested above
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_uri_path_remove_dot_segments() {

  char in[PATH_MAX] = {0};
  char out[PATH_MAX] = {0};
  char *src = NULL;

  strcat(in, "/");

  testrun(!ov_uri_path_remove_dot_segments(NULL, NULL));
  testrun(!ov_uri_path_remove_dot_segments(NULL, out));
  testrun(!ov_uri_path_remove_dot_segments(in, NULL));

  testrun(ov_uri_path_remove_dot_segments(in, out));
  testrun(0 == strncmp(out, "/", 1));

  memset(in, 0, PATH_MAX);
  memset(out, 0, PATH_MAX);
  src = "test";
  strncpy(in, src, strlen(src));
  testrun(ov_uri_path_remove_dot_segments(in, out));
  testrun(0 == strncmp(out, "test", 4));

  memset(in, 0, PATH_MAX);
  memset(out, 0, PATH_MAX);
  src = "/test";
  strncpy(in, src, strlen(src));
  testrun(ov_uri_path_remove_dot_segments(in, out));
  testrun(0 == strncmp(out, "/test", 5));

  memset(in, 0, PATH_MAX);
  memset(out, 0, PATH_MAX);
  src = "///////test";
  strncpy(in, src, strlen(src));
  testrun(ov_uri_path_remove_dot_segments(in, out));
  testrun(0 == strncmp(out, "/test", 5));

  memset(in, 0, PATH_MAX);
  memset(out, 0, PATH_MAX);
  src = "///.//.//test";
  strncpy(in, src, strlen(src));
  testrun(ov_uri_path_remove_dot_segments(in, out));
  testrun(0 == strncmp(out, "/test", 5));

  memset(in, 0, PATH_MAX);
  memset(out, 0, PATH_MAX);
  src = "x/../test";
  strncpy(in, src, strlen(src));
  testrun(ov_uri_path_remove_dot_segments(in, out));
  testrun(0 == strncmp(out, "test", 4));

  memset(in, 0, PATH_MAX);
  memset(out, 0, PATH_MAX);
  src = "/x/../test";
  strncpy(in, src, strlen(src));
  testrun(ov_uri_path_remove_dot_segments(in, out));
  testrun(0 == strncmp(out, "/test", 5));

  memset(in, 0, PATH_MAX);
  memset(out, 0, PATH_MAX);
  src = "/x/y/../../test";
  strncpy(in, src, strlen(src));
  testrun(ov_uri_path_remove_dot_segments(in, out));
  testrun(0 == strncmp(out, "/test", 5));

  memset(in, 0, PATH_MAX);
  memset(out, 0, PATH_MAX);
  src = "/x/y/.././.././test";
  strncpy(in, src, strlen(src));
  testrun(ov_uri_path_remove_dot_segments(in, out));
  testrun(0 == strncmp(out, "/test", 5));

  memset(in, 0, PATH_MAX);
  memset(out, 0, PATH_MAX);
  src = "/x/y/.././.././test";
  strncpy(in, src, strlen(src));
  testrun(ov_uri_path_remove_dot_segments(in, out));
  testrun(0 == strncmp(out, "/test", 5));

  memset(in, 0, PATH_MAX);
  memset(out, 0, PATH_MAX);
  src = "//test";
  strncpy(in, src, strlen(src));
  testrun(ov_uri_path_remove_dot_segments(in, out));
  testrun(0 == strncmp(out, "/test", 5));

  memset(in, 0, PATH_MAX);
  memset(out, 0, PATH_MAX);
  src = "//test";
  strncpy(in, src, strlen(src));
  testrun(ov_uri_path_remove_dot_segments(in, out));
  testrun(0 == strncmp(out, "/test", 5));

  memset(in, 0, PATH_MAX);
  memset(out, 0, PATH_MAX);
  src = "/./test";
  strncpy(in, src, strlen(src));
  testrun(ov_uri_path_remove_dot_segments(in, out));
  testrun(0 == strncmp(out, "/test", 5));

  memset(in, 0, PATH_MAX);
  memset(out, 0, PATH_MAX);
  src = "./test";
  strncpy(in, src, strlen(src));
  testrun(ov_uri_path_remove_dot_segments(in, out));
  testrun(0 == strncmp(out, "test", 4));

  memset(in, 0, PATH_MAX);
  memset(out, 0, PATH_MAX);
  src = "../test";
  strncpy(in, src, strlen(src));
  testrun(!ov_uri_path_remove_dot_segments(in, out));
  testrun(0 == out[0]);

  memset(in, 0, PATH_MAX);
  memset(out, 0, PATH_MAX);
  src = "x/../a";
  strncpy(in, src, strlen(src));
  testrun(ov_uri_path_remove_dot_segments(in, out));
  testrun(0 == strncmp(out, "a", 1));

  memset(in, 0, PATH_MAX);
  memset(out, 0, PATH_MAX);
  src = "../a";
  strncpy(in, src, strlen(src));
  testrun(!ov_uri_path_remove_dot_segments(in, out));
  testrun(0 == out[0]);

  memset(in, 0, PATH_MAX);
  memset(out, 0, PATH_MAX);
  src = "b/../../a";
  strncpy(in, src, strlen(src));
  testrun(!ov_uri_path_remove_dot_segments(in, out));
  testrun(0 == out[0]);

  memset(in, 0, PATH_MAX);
  memset(out, 0, PATH_MAX);
  src = "../b/../a";
  strncpy(in, src, strlen(src));
  testrun(!ov_uri_path_remove_dot_segments(in, out));
  testrun(0 == out[0]);

  memset(in, 0, PATH_MAX);
  memset(out, 0, PATH_MAX);
  src = "x/../../a";
  strncpy(in, src, strlen(src));
  testrun(!ov_uri_path_remove_dot_segments(in, out));
  testrun(0 == out[0]);

  memset(in, 0, PATH_MAX);
  memset(out, 0, PATH_MAX);
  src = "x/../y/.././.././test";
  strncpy(in, src, strlen(src));
  testrun(!ov_uri_path_remove_dot_segments(in, out));
  testrun(0 == out[0]);

  memset(in, 0, PATH_MAX);
  memset(out, 0, PATH_MAX);
  src = "/x/../y/.././.././../test";
  strncpy(in, src, strlen(src));
  testrun(!ov_uri_path_remove_dot_segments(in, out));
  testrun(0 == out[0]);

  memset(in, 0, PATH_MAX);
  memset(out, 0, PATH_MAX);
  src = "../../../../../../../.././.././../x/test";
  strncpy(in, src, strlen(src));
  testrun(!ov_uri_path_remove_dot_segments(in, out));
  testrun(0 == out[0]);

  memset(in, 0, PATH_MAX);
  memset(out, 0, PATH_MAX);
  src = "/x/../y/../././test";
  strncpy(in, src, strlen(src));
  testrun(ov_uri_path_remove_dot_segments(in, out));
  testrun(0 == strncmp(out, "/test", 5));

  memset(in, 0, PATH_MAX);
  memset(out, 0, PATH_MAX);
  src = "a/../../b";
  strncpy(in, src, strlen(src));
  testrun(!ov_uri_path_remove_dot_segments(in, out));
  testrun(0 == out[0]);

  memset(in, 0, PATH_MAX);
  memset(out, 0, PATH_MAX);
  src = "a/../b/../c";
  strncpy(in, src, strlen(src));
  testrun(ov_uri_path_remove_dot_segments(in, out));
  testrun(0 == strncmp(out, "c", 1));

  memset(in, 0, PATH_MAX);
  memset(out, 0, PATH_MAX);
  src = "a//.././b//.//..//./c";
  strncpy(in, src, strlen(src));
  testrun(ov_uri_path_remove_dot_segments(in, out));
  testrun(0 == strncmp(out, "c", 1));

  memset(in, 0, PATH_MAX);
  memset(out, 0, PATH_MAX);
  /* The last / before c will be interpreted as empty path and deleted */
  src = "a//.././b//.//..//.//c";
  strncpy(in, src, strlen(src));
  testrun(ov_uri_path_remove_dot_segments(in, out));
  testrun(0 == strncmp(out, "c", 1));

  memset(in, 0, PATH_MAX);
  memset(out, 0, PATH_MAX);
  /* The first / will not be interpreted as empty path */
  src = "/a//.././b//.//..//./c";
  strncpy(in, src, strlen(src));
  testrun(ov_uri_path_remove_dot_segments(in, out));
  testrun(0 == strncmp(out, "/c", 2));

  memset(in, 0, PATH_MAX);
  memset(out, 0, PATH_MAX);
  /* The first / will not be interpreted as empty path */
  src = "//a//.././b//.//..//./c";
  strncpy(in, src, strlen(src));
  testrun(ov_uri_path_remove_dot_segments(in, out));
  testrun(0 == strncmp(out, "/c", 2));

  memset(in, 0, PATH_MAX);
  memset(out, 0, PATH_MAX);
  /* The first / will not be interpreted as empty path,
   * but the last slash still will */
  src = "//a//.././b//.//..//.//c";
  strncpy(in, src, strlen(src));
  testrun(ov_uri_path_remove_dot_segments(in, out));
  testrun(0 == strncmp(out, "/c", 2));

  memset(in, 0, PATH_MAX);
  memset(out, 0, PATH_MAX);
  src = "//a";
  strncpy(in, src, strlen(src));
  testrun(ov_uri_path_remove_dot_segments(in, out));
  testrun(0 == strncmp(out, "/a", 2));

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

  testrun_test(test_ov_uri_string_is_valid);
  testrun_test(test_ov_uri_from_string);
  testrun_test(test_ov_uri_to_string);

  testrun_test(test_ov_uri_string_is_absolute);
  testrun_test(test_ov_uri_string_is_reference);

  testrun_test(test_ov_uri_string_length);
  testrun_test(test_ov_uri_clear);
  testrun_test(test_ov_uri_free);

  testrun_test(test_ov_uri_path_remove_dot_segments);

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

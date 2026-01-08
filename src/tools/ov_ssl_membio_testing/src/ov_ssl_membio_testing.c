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
        @file           ov_sll_membio_testing.c
        @author         Markus Töpfer

        @date           2022-03-11


        ------------------------------------------------------------------------
*/

#include <openssl/bio.h>
#include <stdio.h>

int main() {
  BIO *b = NULL;
  int len = 0;
  char *out = NULL;

  b = BIO_new(BIO_s_mem());
  len = BIO_write(b, "openssl", 4);
  len = BIO_printf(b, "%s", "zcp");
  len = BIO_ctrl_pending(b);
  printf("len:%d\n", len);
  out = (char *)OPENSSL_malloc(len);
  len = BIO_read(b, out, len);
  printf("out:%s\n", out);
  OPENSSL_free(out);
  BIO_free(b);
  return 0;
}
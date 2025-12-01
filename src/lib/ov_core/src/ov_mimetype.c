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
        @file           ov_mimetype.c
        @author         Markus Toepfer

        @date           2019-07-23

        @ingroup        ov_core

        @brief          Implementation of


        ------------------------------------------------------------------------
*/
#include "ov_mimetype.h"

#include <ov_base/ov_utils.h>
#include <ov_log/ov_log.h>
#include <string.h>

static char *extensions[] = {

    "aac",  "avi",  "bin",  "bmp",   "bz",  "bz2", "css",  "gif",
    "epub", "htm",  "html", "ico",   "ics", "jpg", "jpeg", "js",
    "json", "ogg",  "oga",  "ogv",   "ogx", "png", "pdf",  "sh",
    "svg",  "tar",  "tif",  "tiff",  "ttf", "txt", "wav",  "weba",
    "webm", "webp", "woff", "woff2", "xml", "zip", "7z",   "mp3"};

static char *mime[] = {
    "audio/aac",       "video/x-msvideo",    "application/octet-stream",
    "image/bmp",       "application/x-bzip", "application/x-bzip2",
    "text/css",        "image/gif",          "application/epub+zip",
    "text/html",       "text/html",          "image/vnd.microsoft.icon",
    "text/calendar",   "image/jpeg",         "image/jpeg",
    "text/javascript", "application/json",   "audio/ogg",
    "audio/ogg",       "video/ogg",          "application/ogg",
    "image/png",       "application/pdf",    "application/x-sh",
    "image/svg+xml",   "application/x-tar",  "image/tiff",
    "image/tiff",      "font/ttf",           "text/plain",
    "audio/wav",       "audio/webm",         "video/webm",
    "image/webp",      "font/woff",          "font/woff2",
    "text/xml",        "application/zip",    "application/x-7z-compressed",
    "audio/mpeg",
};

/*----------------------------------------------------------------------------*/

const char *ov_mimetype_from_file_extension(const char *ext, size_t size) {

  char *out = NULL;

  if (!ext || size < 1)
    goto done;

  size_t ext_size = sizeof(extensions) / sizeof(extensions[0]);
  size_t mime_size = sizeof(mime) / sizeof(mime[0]);

  // ov_log_debug("%i | %i", ext_size, mime_size);

  OV_ASSERT(ext_size == mime_size);

  size_t len = 0;

  for (size_t i = 0; i < ext_size; i++) {

    len = strlen(extensions[i]);
    if (len != size)
      continue;

    if (0 != strncmp(extensions[i], ext, size))
      continue;

    out = mime[i];
    break;
  }

done:
  return out;
}

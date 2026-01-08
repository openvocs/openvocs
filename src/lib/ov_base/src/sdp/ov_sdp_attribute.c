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
        @file           ov_sdp_attribute.c
        @author         Markus Töpfer

        @date           2019-12-0

        @ingroup        ov_sdp_attribute

        @brief          Implementation of SDP attribute access functions

        ------------------------------------------------------------------------
*/
#include "../../include/ov_sdp_attribute.h"
#include "../../include/ov_sdp_grammar.h"

#include "../../include/ov_convert.h"

#include "../../include/ov_config_keys.h"
#include "../../include/ov_sdp.h"

bool ov_sdp_attribute_is_set(const ov_sdp_list *attributes, const char *name) {

  if (!attributes || !name)
    goto error;

  size_t len = strlen(name);

  ov_sdp_list *attr = (ov_sdp_list *)attributes;

  while (attr) {

    if (!attr->key) {
      ov_log_error("SDP attribute without key");
      goto error;
    }

    if (strlen(attr->key) == len)
      if (0 == strncmp(attr->key, name, len))
        return true;

    attr = ov_node_next(attr);
  }
error:
  return false;
}

/*----------------------------------------------------------------------------*/

bool ov_sdp_attribute_del(ov_sdp_list **attributes, const char *name) {

  if (!attributes || !name)
    goto error;

  size_t len = strlen(name);

  ov_sdp_list *attr = *attributes;
  ov_sdp_list *del = NULL;

  while (attr) {

    if (!attr->key) {
      ov_log_error("SDP attribute without key");
      goto error;
    }

    del = NULL;

    if ((strlen(attr->key) == len) && (0 == strncmp(attr->key, name, len)))
      del = attr;

    attr = ov_node_next(attr);

    if (NULL == del)
      continue;

    if (!ov_node_unplug((void **)attributes, del))
      goto error;

    del = ov_sdp_list_free(del);
  }

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

const char *ov_sdp_attribute_get(const ov_sdp_list *attributes,
                                 const char *name) {

  if (!attributes || !name)
    goto error;

  size_t len = strlen(name);

  ov_sdp_list *attr = (ov_sdp_list *)attributes;

  while (attr) {

    if (!attr->key) {
      ov_log_error("SDP attribute without key");
      goto error;
    }

    if (strlen(attr->key) == len)
      if (0 == strncmp(attr->key, name, len))
        return attr->value;

    attr = ov_node_next(attr);
  }
error:
  return false;
}

/*----------------------------------------------------------------------------*/

bool ov_sdp_attribute_add(ov_sdp_list **attributes, const char *key,
                          const char *val) {

  ov_sdp_list *list = NULL;

  if (!attributes || !key)
    goto error;

  /*
   *      (1) validate input
   */

  if (!ov_sdp_is_token(key, strlen(key)))
    goto error;
  if (val)
    if (!ov_sdp_is_byte_string(val, strlen(val)))
      goto error;

  /*
   *      (2) create list node
   */

  list = ov_sdp_list_create();
  if (!list)
    goto error;

  list->key = key;
  list->value = val;

  /*
   *      (3) add list node to head
   */

  if (!ov_node_push((void **)attributes, list))
    goto error;

  return true;
error:
  list = ov_sdp_list_free(list);
  return false;
}

/*----------------------------------------------------------------------------*/

bool ov_sdp_is_recvonly(const ov_sdp_description *description) {

  if (!description)
    return false;
  return ov_sdp_attribute_is_set(description->attributes, OV_SDP_RECV_ONLY);
}

/*----------------------------------------------------------------------------*/

bool ov_sdp_is_sendrecv(const ov_sdp_description *description) {

  if (!description)
    return false;
  return ov_sdp_attribute_is_set(description->attributes, OV_SDP_SEND_RECV);
}

/*----------------------------------------------------------------------------*/

bool ov_sdp_is_sendonly(const ov_sdp_description *description) {

  if (!description)
    return false;
  return ov_sdp_attribute_is_set(description->attributes, OV_SDP_SEND_ONLY);
}

/*----------------------------------------------------------------------------*/

bool ov_sdp_is_inactive(const ov_sdp_description *description) {

  if (!description)
    return false;
  return ov_sdp_attribute_is_set(description->attributes, OV_SDP_INACTIVE);
}

/*----------------------------------------------------------------------------*/

const char *ov_sdp_get_orientation(const ov_sdp_description *description) {

  if (!description)
    return NULL;
  return ov_sdp_attribute_get(description->attributes, OV_SDP_ORIENTATION);
}

/*----------------------------------------------------------------------------*/

const char *ov_sdp_get_type(const ov_sdp_description *description) {

  if (!description)
    return NULL;
  return ov_sdp_attribute_get(description->attributes, OV_SDP_TYPE);
}

/*----------------------------------------------------------------------------*/

const char *ov_sdp_get_charset(const ov_sdp_description *description) {

  if (!description)
    return NULL;
  return ov_sdp_attribute_get(description->attributes, OV_SDP_CHARSET);
}

/*----------------------------------------------------------------------------*/

const char *ov_sdp_get_sdplang(const ov_sdp_description *description) {

  if (!description)
    return NULL;
  return ov_sdp_attribute_get(description->attributes, OV_SDP_SDPLANG);
}

/*----------------------------------------------------------------------------*/

const char *ov_sdp_get_lang(const ov_sdp_description *description) {

  if (!description)
    return NULL;
  return ov_sdp_attribute_get(description->attributes, OV_SDP_LANG);
}

/*----------------------------------------------------------------------------*/

static uint64_t get_number(const char *string) {

  if (!string)
    return 0;

  uint64_t nbr = 0;

  if (!ov_convert_string_to_uint64(string, strlen(string), &nbr))
    return 0;

  return nbr;
}

/*----------------------------------------------------------------------------*/

uint64_t ov_sdp_get_framerate(const ov_sdp_description *description) {

  if (!description)
    return 0;
  return get_number(
      ov_sdp_attribute_get(description->attributes, OV_SDP_FRAMERATE));
}

/*----------------------------------------------------------------------------*/

uint64_t ov_sdp_get_quality(const ov_sdp_description *description) {

  if (!description)
    return 0;
  return get_number(
      ov_sdp_attribute_get(description->attributes, OV_SDP_QUALITY));
}

/*----------------------------------------------------------------------------*/

const char *ov_sdp_get_fmtp(const ov_sdp_description *description,
                            const char *name) {

  if (!description || !name)
    goto error;

  size_t len = strlen(name);

  ov_sdp_list *attr = description->attributes;

  while (attr) {

    if (!attr->key) {
      ov_log_error("SDP attribute without key");
      goto error;
    }

    if ((attr->key[0] == 'f') &&
        (0 == strncmp(attr->key, OV_SDP_FMTP, strlen(OV_SDP_FMTP)))) {

      if ((0 == strncmp(attr->value, name, len))) {

        char *ptr = memchr(attr->value, ' ', strlen(attr->value));
        ptr += 1;

        return ptr;
      }
    }

    attr = ov_node_next(attr);
  }
error:
  return false;
}

/*----------------------------------------------------------------------------*/

bool ov_sdp_del_fmtp(const ov_sdp_description *description, const char *name) {

  if (!description || !name)
    goto error;

  size_t len = strlen(name);

  ov_sdp_list *attr = description->attributes;

  while (attr) {

    if (!attr->key)
      goto error;

    if ((attr->key[0] == 'f') &&
        (0 == strncmp(attr->key, OV_SDP_FMTP, strlen(OV_SDP_FMTP)))) {

      if ((0 == strncmp(attr->value, name, len))) {

        if (!ov_node_unplug((void **)&description->attributes, attr))
          goto error;

        attr = ov_sdp_list_free(attr);

        return true;
      }
    }

    attr = ov_node_next(attr);
  }

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

const char *ov_sdp_get_rtpmap(const ov_sdp_description *description,
                              const char *name) {

  if (!description || !name)
    goto error;

  size_t len = strlen(name);

  ov_sdp_list *attr = description->attributes;

  while (attr) {

    if (!attr->key) {
      ov_log_error("SDP attribute without key");
      goto error;
    }

    if ((attr->key[0] == 'r') &&
        (0 == strncmp(attr->key, OV_SDP_RTPMAP, strlen(OV_SDP_RTPMAP)))) {

      if ((0 == strncmp(attr->value, name, len))) {

        char *ptr = memchr(attr->value, ' ', strlen(attr->value));
        ptr += 1;

        return ptr;
      }
    }

    attr = ov_node_next(attr);
  }
error:
  return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_sdp_del_rtpmap(const ov_sdp_description *description,
                       const char *name) {

  if (!description || !name)
    goto error;

  size_t len = strlen(name);

  ov_sdp_list *attr = description->attributes;

  while (attr) {

    if (!attr->key) {
      ov_log_error("SDP attribute without key");
      goto error;
    }

    if ((attr->key[0] == 'r') &&
        (0 == strncmp(attr->key, OV_SDP_RTPMAP, strlen(OV_SDP_RTPMAP)))) {

      if ((0 == strncmp(attr->value, name, len))) {

        if (!ov_node_unplug((void **)&description->attributes, attr))
          goto error;

        attr = ov_sdp_list_free(attr);
        return true;
      }
    }

    attr = ov_node_next(attr);
  }

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

bool ov_sdp_attributes_iterate(ov_sdp_list **attributes, const char *name,
                               const char **out) {

  if (!attributes || !name || !out)
    goto error;

  ov_sdp_list *attr = *attributes;

  size_t len = strlen(name);
  while (attr) {

    if (!attr->key) {
      ov_log_error("SDP attribute without key");
      goto error;
    }

    if ((len == strlen(attr->key)) && (0 == strncmp(attr->key, name, len))) {
      *out = attr->value;

      attr = ov_node_next(attr);
      *attributes = attr;
      return true;
    }

    attr = ov_node_next(attr);
  }

  *out = NULL;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

bool ov_sdp_add_fmt(ov_sdp_description *desc, const char *fmt) {

  if (!desc || !fmt)
    goto error;

  ov_sdp_list *item = ov_sdp_list_create();
  if (!item)
    goto error;

  item->value = fmt;
  if (!ov_node_push((void **)&desc->media.formats, item)) {
    item = ov_sdp_list_free(item);
    goto error;
  }

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_sdp_rtpmap_to_json(const ov_sdp_description *desc,
                                     const char *fmt) {

  ov_json_value *out = NULL;
  ov_json_value *val = NULL;

  if (!desc || !fmt)
    goto error;

  const char *map = ov_sdp_get_rtpmap(desc, fmt);
  if (!map)
    goto error;

  out = ov_json_object();
  if (!out)
    goto error;

  size_t len = strlen(map);
  size_t plen = 0;

  char *ptr = memchr(map, '/', len);

  if (!ptr) {
    plen = len;
  } else {
    plen = ptr - map;
  }

  val = ov_json_string(NULL);
  if (!ov_json_string_set_length(val, map, plen))
    goto error;

  if (!ov_json_object_set(out, OV_KEY_NAME, val))
    goto error;

  char *rate = ptr + 1;
  ptr = memchr(rate, '/', len - (rate - map));

  if (ptr) {
    plen = ptr - rate;
  } else {
    plen = len - (rate - map);
  }

  uint64_t number = 0;

  if (!ov_convert_string_to_uint64(rate, plen, &number))
    goto error;

  val = ov_json_number(number);
  if (!ov_json_object_set(out, OV_KEY_SAMPLE_RATE_HERTZ, val))
    goto error;

  if (ptr) {

    ptr++;
    plen = len - (ptr - map);
  }

  val = ov_json_string(NULL);
  if (!ov_json_string_set_length(val, ptr, plen))
    goto error;

  if (!ov_json_object_set(out, OV_KEY_PARAMETER, val))
    goto error;

  return out;
error:
  ov_json_value_free(out);
  ov_json_value_free(val);
  return NULL;
}

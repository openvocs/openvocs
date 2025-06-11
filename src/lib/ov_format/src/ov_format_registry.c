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

        @author         Michael J. Beer, DLR/GSOC

        ------------------------------------------------------------------------
*/

#include <ov_base/ov_dict.h>

#include <ov_base/ov_utils.h>

#include "../include/ov_format_ethernet.h"
#include "../include/ov_format_ipv4.h"
#include "../include/ov_format_ipv6.h"
#include "../include/ov_format_linux_sll.h"
#include "../include/ov_format_pcap.h"
#include "../include/ov_format_registry.h"
#include "../include/ov_format_rtp.h"
#include "../include/ov_format_udp.h"

/*----------------------------------------------------------------------------*/

/**
 * Unfortunately required for forward decl. in header
 */
struct ov_format_registry_struct {

    int dummy;
};

static ov_format_registry *g_format_registry = 0;

/*----------------------------------------------------------------------------*/

static void *free_format_handler(void *fth) {

    if (0 == fth) return fth;

    free(fth);

    return 0;
}

/*----------------------------------------------------------------------------*/

static ov_dict *get_format_registry(ov_format_registry *registry) {

    if (0 != registry) return (ov_dict *)registry;

    if (0 == g_format_registry) {

        g_format_registry = ov_format_registry_create();
        OV_ASSERT(0 != g_format_registry);
    }

    return (ov_dict *)g_format_registry;
}

/*----------------------------------------------------------------------------*/

ov_format *ov_format_as(ov_format *f,
                        char const *type,
                        void *options,
                        ov_format_registry *registry) {

    if (0 == type) goto error;

    if (0 == f) {

        ov_log_error("No lower format given");
        goto error;
    }

    if (0 == registry) {
        registry = g_format_registry;
    }

    if (0 == registry) goto error;

    ov_dict *registry_dict = (ov_dict *)registry;

    ov_format_handler *handler = ov_dict_get(registry_dict, type);

    if (0 == handler) {

        ov_log_error("No format handler for %s found", type);
        goto error;
    }

    return ov_format_wrap(f, type, handler, options);

error:

    return 0;
}

/*----------------------------------------------------------------------------*/

bool ov_format_registry_register_type(char const *type,
                                      ov_format_handler handler,
                                      ov_format_registry *registry) {

    ov_format_handler *handler_copy = 0;
    char *ft_copy = 0;

    if (0 == type) {

        ov_log_error("Null pointer for format type name");
        goto error;
    }

    if (strlen(type) > OV_FORMAT_TYPE_MAX_STR_LEN) {

        ov_log_error("Type string too long: Max %zu are allowed",
                     (size_t)OV_FORMAT_TYPE_MAX_STR_LEN);
        goto error;
    }

    if ((0 != handler.create_data) && (0 == handler.free_data)) {

        ov_log_error(
            "Cannot register a format type with 'create_data' set but "
            "'free_data' not set");
        goto error;
    }

    ov_dict *registry_dict = get_format_registry(registry);

    if (ov_dict_is_set(registry_dict, type)) {
        goto error;
    }

    handler_copy = calloc(1, sizeof(ov_format_handler));
    OV_ASSERT(0 != handler_copy);

    *handler_copy = handler;

    ft_copy = strndup(type, 255 + 1);

    if (0 == ft_copy) {

        ov_log_error("Could not copy format type name");
        goto error;
    }

    if (255 == strnlen(ft_copy, 255)) {

        ov_log_error(
            "Could not copy format type name - too long(Max 255 chars "
            "supported)");
        goto error;
    }

    bool success = ov_dict_set(registry_dict, ft_copy, handler_copy, 0);

    if (!success) goto error;

    return success;

error:

    if (0 != handler_copy) {

        free(handler_copy);
        handler_copy = 0;
    }

    if (0 != ft_copy) {

        free(ft_copy);
        ft_copy = 0;
    }

    OV_ASSERT(0 == handler_copy);
    OV_ASSERT(0 == ft_copy);

    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_format_registry_unregister_type(char const *format_type,
                                        ov_format_handler *removed_handler,
                                        ov_format_registry *registry) {

    if (0 == format_type) {

        ov_log_error("No type to unregister given");
        goto error;
    }

    if (0 == registry) {

        registry = g_format_registry;
    }

    if (0 == registry) {

        /* There is no global registry yet ... */
        return true;
    }

    OV_ASSERT(0 != format_type);

    ov_dict *dict = (ov_dict *)registry;

    ov_format_handler *old = 0;

    if (0 != removed_handler) {

        old = ov_dict_get(dict, format_type);
    }

    if (0 != old) {

        OV_ASSERT(0 != removed_handler);
        memcpy(removed_handler, old, sizeof(ov_format_handler));
    }

    return ov_dict_del(dict, format_type);

error:

    return false;
}

/*----------------------------------------------------------------------------*/

ov_format_registry *ov_format_registry_clear(ov_format_registry *registry) {

    if (0 != registry) {

        return ov_dict_free((ov_dict *)registry);
    }

    if (0 != g_format_registry) {

        g_format_registry =
            (ov_format_registry *)ov_dict_free((ov_dict *)g_format_registry);
    }

    return g_format_registry;
}

/*----------------------------------------------------------------------------*/

ov_format_registry *ov_format_registry_create() {

    ov_dict_config config = ov_dict_string_key_config(10);
    config.value.data_function.free = free_format_handler;

    return (ov_format_registry *)ov_dict_create(config);
}

/*----------------------------------------------------------------------------*/

bool ov_format_registry_register_default(ov_format_registry *registry) {

    bool ok = ov_format_pcap_install(registry);
    ok = ok && ov_format_ethernet_install(registry);
    ok = ok && ov_format_ethernet_dispatcher_install(registry);
    ok = ok && ov_format_linux_sll_install(registry);
    ok = ok && ov_format_ipv4_install(registry);
    ok = ok && ov_format_ipv6_install(registry);
    ok = ok && ov_format_udp_install(registry);
    ok = ok && ov_format_rtp_install(registry);

    return ok;
}

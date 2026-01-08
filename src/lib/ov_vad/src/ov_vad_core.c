/***
        ------------------------------------------------------------------------

        Copyright (c) 2025 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_vad_core.c
        @author         Markus Töpfer

        @date           2025-01-10


        ------------------------------------------------------------------------
*/
#include "../include/ov_vad_core.h"
#include "../include/ov_vad_thread_msg.h"

#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_dict.h>
#include <ov_base/ov_linked_list.h>
#include <ov_base/ov_mc_socket.h>
#include <ov_base/ov_rtp_frame.h>
#include <ov_base/ov_string.h>
#include <ov_base/ov_thread_lock.h>
#include <ov_base/ov_thread_loop.h>
#include <ov_base/ov_time.h>
#include <ov_codec/ov_codec.h>
#include <ov_codec/ov_codec_opus.h>
#include <ov_pcm16s/ov_pcm16_mod.h>

#define OV_VAD_CORE_MAGIC_BYTES 0xfe21
#define OV_VAD_CODEC_GC_USECS 5000000
#define OV_VAD_CODEC_OUTDATED_SECS 5

/*---------------------------------------------------------------------------*/

struct ov_vad_core {

    uint16_t magic_bytes;
    ov_vad_core_config config;

    ov_dict *loops;
    uint32_t idle_check;

    struct {

        ov_codec_factory *factory;
        ov_dict *codecs;

        uint32_t gc_timer;

    } codec;
};

/*---------------------------------------------------------------------------*/

typedef struct Counter {

    uint32_t on;
    uint32_t off;
    uint64_t last_active;
    bool active;

} Counter;

/*---------------------------------------------------------------------------*/

typedef struct Loops {

    ov_vad_core *core;
    int socket;
    ov_socket_configuration config;
    char *name;
    bool on;

    struct {

        uint32_t on;
        uint32_t off;

    } count;

    ov_dict *ssrcs;

} Loops;

/*---------------------------------------------------------------------------*/

static void *loop_data_free(void *self) {

    Loops *loop = (Loops *)self;
    loop->name = ov_data_pointer_free(loop->name);

    if (loop->socket > 0) {

        ov_event_loop_unset(loop->core->config.loop, loop->socket, NULL);

        close(loop->socket);
    }

    loop->ssrcs = ov_dict_free(loop->ssrcs);

    loop = ov_data_pointer_free(loop);

    return NULL;
}

/*----------------------------------------------------------------------------*/

typedef struct {

    ov_codec *codec;
    time_t last_used_epoch_secs; // For garbage collection

} codec_entry;

/*----------------------------------------------------------------------------*/

static ov_codec *codec_from_codec_entry(codec_entry *entry,
                                        uint64_t now_epoch_secs) {

    if (0 != entry) {

        entry->last_used_epoch_secs = now_epoch_secs;
        return entry->codec;

    } else {

        return 0;
    }
}

/*----------------------------------------------------------------------------*/

static codec_entry *codec_entry_free(codec_entry *entry) {

    if (0 != entry) {
        entry->codec = ov_codec_free(entry->codec);
        return ov_free(entry);
    }

    return entry;
}

/*---------------------------------------------------------------------------*/

static void *codec_entry_free_void(void *codec) {

    return (void *)codec_entry_free(codec);
}

/*----------------------------------------------------------------------------*/

static ov_codec *get_codec_ssrc(ov_vad_core *self, uint32_t ssrc) {

    intptr_t key = ssrc;
    codec_entry *codec = 0;

    if (ov_ptr_valid(self, "Cannot get Codec - invalid vad pointer")) {

        codec = ov_dict_get(self->codec.codecs, (void *)key);

        if (!codec) {

            codec = calloc(1, sizeof(codec_entry));

            codec->codec = ov_codec_factory_get_codec(
                self->codec.factory, ov_codec_opus_id(), key, 0);

            if (codec &&
                (!ov_dict_set(self->codec.codecs, (void *)key, codec, 0))) {
                codec = codec_entry_free(codec);
            }
        }
    }

    return codec_from_codec_entry(codec, time(0));
}

/*---------------------------------------------------------------------------*/

static bool init_config(ov_vad_core_config *config) {

    if (!config)
        goto error;

    if (0 == config->limits.frames_activate) {

        config->limits.frames_activate = 3;
    }

    if (0 == config->limits.frames_deactivate) {

        config->limits.frames_deactivate = 25;
    }

    return true;
error:
    return false;
}

/*---------------------------------------------------------------------------*/

struct container2 {

    bool all_off;
    uint64_t now;
};

/*---------------------------------------------------------------------------*/

static bool check_all_inactive(const void *key, void *val, void *data) {

    if (!key)
        return true;

    Counter *counter = (Counter *)val;
    struct container2 *container = (struct container2 *)data;

    if (container->now - counter->last_active > 200000)
        counter->active = false;

    if (counter->active)
        container->all_off = false;

    return true;
}

/*---------------------------------------------------------------------------*/

static bool handle_loop_io(ov_vad_core *self, Loops *loop, uint8_t *buf,
                           size_t size) {

    int16_t pcm16[2048] = {0};

    ov_rtp_frame *frame = ov_rtp_frame_decode(buf, size);
    if (!frame)
        goto error;

    ov_codec *stream_codec = get_codec_ssrc(self, frame->expanded.ssrc);

    int32_t length_bytes = 0;

    if (stream_codec)
        length_bytes = ov_codec_decode(
            stream_codec, frame->expanded.sequence_number,
            frame->expanded.payload.data, frame->expanded.payload.length,
            (uint8_t *)pcm16, 2048);

    if (0 > length_bytes)
        goto done;

    ov_vad_parameters vad_params = {0};
    ov_pcm_16_get_vad_parameters(length_bytes / 2, pcm16, &vad_params);

    Counter *counter =
        ov_dict_get(loop->ssrcs, (void *)(intptr_t)frame->expanded.ssrc);

    if (!counter) {

        counter = calloc(1, sizeof(Counter));
        if (!counter)
            goto error;

        ov_dict_set(loop->ssrcs, (void *)(intptr_t)frame->expanded.ssrc,
                    counter, NULL);
    }

    counter->last_active = ov_time_get_current_time_usecs();

    OV_ASSERT(counter);

    bool switch_on = false;

    if (ov_pcm_vad_detected(48000, vad_params, self->config.vad)) {

        counter->off = 0;

        if (!counter->active) {

            counter->on++;

            if (counter->on >= self->config.limits.frames_activate) {

                // voice switch on
                ov_log_debug("VAD on %s SSRC %i", loop->name,
                             frame->expanded.ssrc);

                counter->active = true;
                counter->on = 0;

                switch_on = true;
            }
        }

    } else {

        if (counter->active) {

            counter->off++;

            if (counter->off >= self->config.limits.frames_deactivate) {
                // voice switch off

                ov_log_debug("VAD off %s SSRC %i", loop->name,
                             frame->expanded.ssrc);

                counter->off = 0;
                counter->active = false;
            }
        }
    }

    if (switch_on) {

        if (!loop->on) {

            loop->on = true;
            self->config.callbacks.vad(self->config.callbacks.userdata,
                                       loop->name, loop->on);
        }
    }

    struct container2 container = (struct container2){
        .all_off = true, .now = ov_time_get_current_time_usecs()};

    ov_dict_for_each(loop->ssrcs, &container, check_all_inactive);

    if (container.all_off) {

        if (loop->on) {

            loop->on = false;
            self->config.callbacks.vad(self->config.callbacks.userdata,
                                       loop->name, loop->on);
        }
    }

done:
    frame = ov_rtp_frame_free(frame);
    return true;
error:
    return false;
}

/*---------------------------------------------------------------------------*/

struct container1 {

    time_t now;
    ov_list *outdated;
};

/*---------------------------------------------------------------------------*/

static bool check_outdated(const void *key, void *val, void *data) {

    if (!key)
        return true;

    struct container1 *container = (struct container1 *)data;
    codec_entry *entry = (codec_entry *)val;

    if ((container->now - entry->last_used_epoch_secs) >
        OV_VAD_CODEC_OUTDATED_SECS)
        ov_list_push(container->outdated, (void *)key);

    return true;
}

/*---------------------------------------------------------------------------*/

static bool drop_outdated(void *item, void *data) {

    ov_vad_core *self = ov_vad_core_cast(data);
    return ov_dict_del(self->codec.codecs, item);
}

/*---------------------------------------------------------------------------*/

static bool drop_ssrcs(const void *key, void *val, void *data) {

    if (!key)
        return true;
    UNUSED(data);

    Loops *loop = (Loops *)val;
    ov_dict_clear(loop->ssrcs);
    return true;
}

/*---------------------------------------------------------------------------*/

static bool run_codec_gc(uint32_t timer, void *userdata) {

    UNUSED(timer);
    ov_vad_core *self = ov_vad_core_cast(userdata);

    ov_list *outdated = ov_linked_list_create((ov_list_config){0});

    struct container1 container =
        (struct container1){.now = time(0), .outdated = outdated};

    ov_dict_for_each(self->codec.codecs, &container, check_outdated);

    ov_list_for_each(outdated, self, drop_outdated);

    outdated = ov_list_free(outdated);

    ov_dict_for_each(self->loops, NULL, drop_ssrcs);

    self->codec.gc_timer = ov_event_loop_timer_set(
        self->config.loop, OV_VAD_CODEC_GC_USECS, self, run_codec_gc);

    return true;
}

/*---------------------------------------------------------------------------*/

static bool check_idle(const void *key, void *val, void *data) {

    if (!key)
        return true;
    ov_vad_core *self = ov_vad_core_cast(data);

    Loops *loop = (Loops *)val;

    struct container2 container = (struct container2){
        .all_off = true, .now = ov_time_get_current_time_usecs()};

    ov_dict_for_each(loop->ssrcs, &container, check_all_inactive);

    if (container.all_off) {

        if (loop->on) {

            loop->on = false;
            self->config.callbacks.vad(self->config.callbacks.userdata,
                                       loop->name, loop->on);
        }
    }

    return true;
}

/*---------------------------------------------------------------------------*/

static bool check_ilde_loops(uint32_t timer, void *userdata) {

    UNUSED(timer);
    ov_vad_core *self = ov_vad_core_cast(userdata);

    self->idle_check = ov_event_loop_timer_set(self->config.loop, 500000, self,
                                               check_ilde_loops);

    ov_dict_for_each(self->loops, self, check_idle);

    return true;
}

/*---------------------------------------------------------------------------*/

ov_vad_core *ov_vad_core_create(ov_vad_core_config config) {

    ov_vad_core *vad = NULL;

    if (!init_config(&config))
        goto error;

    vad = calloc(1, sizeof(ov_vad_core));
    if (!vad)
        goto error;

    vad->magic_bytes = OV_VAD_CORE_MAGIC_BYTES;
    vad->config = config;

    ov_dict_config d_config = ov_dict_intptr_key_config(255);
    d_config.value.data_function.free = loop_data_free;

    vad->loops = ov_dict_create(d_config);

    vad->codec.factory = ov_codec_factory_create_standard();

    d_config = ov_dict_intptr_key_config(255);
    d_config.value.data_function.free = codec_entry_free_void;
    vad->codec.codecs = ov_dict_create(d_config);
    if (!vad->codec.codecs)
        goto error;

    vad->codec.gc_timer = ov_event_loop_timer_set(
        vad->config.loop, OV_VAD_CODEC_GC_USECS, vad, run_codec_gc);

    vad->idle_check = ov_event_loop_timer_set(vad->config.loop, 500000, vad,
                                              check_ilde_loops);

    return vad;
error:
    ov_vad_core_free(vad);
    return NULL;
}

/*---------------------------------------------------------------------------*/

ov_vad_core *ov_vad_core_free(ov_vad_core *self) {

    if (!ov_vad_core_cast(self))
        return self;

    self->loops = ov_dict_free(self->loops);

    self->codec.factory = ov_codec_factory_free(self->codec.factory);

    self = ov_data_pointer_free(self);
    return NULL;
}

/*---------------------------------------------------------------------------*/

ov_vad_core *ov_vad_core_cast(const void *data) {

    if (!data)
        return NULL;

    if (*(uint16_t *)data != OV_VAD_CORE_MAGIC_BYTES)
        return NULL;

    return (ov_vad_core *)data;
}

/*---------------------------------------------------------------------------*/

ov_vad_core_config ov_vad_core_config_from_json(const ov_json_value *in) {

    ov_vad_core_config config = {0};

    ov_json_value *limits = ov_json_object_get(in, OV_KEY_LIMITS);

    config.limits.frames_activate =
        ov_json_number_get(ov_json_get(limits, "/" OV_KEY_ACTIVATE));

    config.limits.frames_deactivate =
        ov_json_number_get(ov_json_get(limits, "/" OV_KEY_DEACTIVATE));

    return config;
}

/*---------------------------------------------------------------------------*/

static bool callback_io_loop(int socket, uint8_t events, void *userdata) {

    uint8_t buf[OV_UDP_PAYLOAD_OCTETS] = {0};

    ov_vad_core *self = ov_vad_core_cast(userdata);

    Loops *loop = ov_dict_get(self->loops, (void *)(intptr_t)socket);
    if (!loop)
        goto error;

    if ((events & OV_EVENT_IO_ERR) || (events & OV_EVENT_IO_CLOSE)) {

        ov_log_debug("Lost VAD socket for loop %s", loop->name);
        ov_dict_del(self->loops, (void *)(intptr_t)socket);

        goto error;
    }

    ssize_t bytes = recv(socket, buf, OV_UDP_PAYLOAD_OCTETS, 0);
    if (-1 == bytes) {
        goto done;
    }

    handle_loop_io(self, loop, buf, bytes);

done:
    return true;
error:
    return false;
}

/*---------------------------------------------------------------------------*/

bool ov_vad_core_add_loop(ov_vad_core *self, const char *name,
                          ov_socket_configuration config) {

    if (!self || !name)
        goto error;

    int socket = ov_mc_socket(config);
    if (-1 == socket)
        goto error;

    if (!ov_socket_ensure_nonblocking(socket))
        goto error;

    if (!ov_event_loop_set(self->config.loop, socket,
                           OV_EVENT_IO_IN | OV_EVENT_IO_ERR | OV_EVENT_IO_CLOSE,
                           self, callback_io_loop)) {
        goto error;
    }

    Loops *loop = calloc(1, sizeof(Loops));
    if (!loop)
        goto error;

    loop->socket = socket;
    loop->config = config;
    loop->name = ov_string_dup(name);
    loop->core = self;
    loop->on = false;
    loop->count.on = 0;
    loop->count.off = 0;

    ov_dict_config d_config = ov_dict_intptr_key_config(255);
    d_config.value.data_function.free = ov_data_pointer_free;
    loop->ssrcs = ov_dict_create(d_config);

    if (!ov_dict_set(self->loops, (void *)(intptr_t)socket, loop, NULL)) {
        loop_data_free(loop);
        goto error;
    }

    ov_log_debug("VAD added loop %s", loop->name);

    return true;
error:
    return false;
}

/*---------------------------------------------------------------------------*/

bool ov_vad_core_set_vad(ov_vad_core *self, ov_vad_config config) {

    if (!self)
        return false;

    ov_log_debug("Set vad config %f %f ",
                 config.zero_crossings_rate_threshold_hertz,
                 config.powerlevel_density_threshold_db);

    self->config.vad = config;
    return true;
}
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
        @file           ov_signaling_app.c
        @author         Markus Toepfer
        @author         Michael Beer

        @date           2019-05-02


        ------------------------------------------------------------------------
*/
#include "../include/ov_signaling_app.h"
#include <ov_base/ov_utils.h>

#define IMPL_SIGNALING_APP_MAGIC_BYTES 0xbbaa
#define IMPL_SHUTDOWN_DELAY 10000 // 10ms

#include <ov_base/ov_dict.h>
#include <ov_base/ov_event_keys.h>
#include <ov_base/ov_json.h>

/*----------------------------------------------------------------------------*/

typedef struct {

  uint16_t magic_bytes;
  void *userdata;

  struct {

    ov_dict *functions;
    ov_json_value *descriptions;

  } command;

  ov_app *(*free)(ov_app *self);

  void (*monitor)(void *, ov_direction direction, int local_socket,
                  const ov_socket_data *remote, const ov_json_value *value);

  void *monitor_userdata;

  bool (*original_send)(ov_app *self, int socket, const ov_socket_data *remote,
                        void *data);

} SignalingApp;

/*----------------------------------------------------------------------------*/

static SignalingApp *signaling_app_cast(void *data) {

  if (!data)
    return NULL;
  if (*(uint16_t *)data == IMPL_SIGNALING_APP_MAGIC_BYTES)
    return (SignalingApp *)data;

  return NULL;
}

/*----------------------------------------------------------------------------*/

static ov_app *impl_signaling_app_free(ov_app *self);

/* Beware: Works only with JSON data ! */
static bool impl_signaling_app_send(ov_app *app, int socket,
                                    const ov_socket_data *remote, void *data);

/*----------------------------------------------------------------------------*/

static void default_monitor(void *userdata, ov_direction direction,
                            int local_socket, const ov_socket_data *remote,
                            const ov_json_value *value) {

  UNUSED(userdata);
  UNUSED(direction);
  UNUSED(local_socket);
  UNUSED(remote);
  UNUSED(value);

  // ov_log_info("IO %s", (direction == OV_IN) ? "IN" : "OUT");
}

/*----------------------------------------------------------------------------*/

ov_app *ov_signaling_app_create(ov_app_config config) {

  SignalingApp *custom = NULL;
  ov_app *app = NULL;

  if (!config.loop)
    goto error;

  app = ov_app_create(config);
  if (!app)
    goto error;

  custom = calloc(1, sizeof(SignalingApp));
  if (!custom)
    goto error;

  custom->magic_bytes = IMPL_SIGNALING_APP_MAGIC_BYTES;

  custom->userdata = config.userdata;
  app->config.userdata = custom;

  custom->free = app->free;
  app->free = impl_signaling_app_free;

  custom->command.functions = ov_dict_create(ov_dict_string_key_config(255));
  if (!custom->command.functions)
    goto error;

  custom->command.descriptions = ov_json_object();

  if (!custom->command.descriptions)
    goto error;

  custom->monitor = default_monitor;
  custom->monitor_userdata = 0;

  custom->original_send = app->send;
  app->send = impl_signaling_app_send;

  return app;
error:
  if (app)
    ov_app_free(app);
  return NULL;
}

/*----------------------------------------------------------------------------*/

void *ov_signaling_app_get_userdata(ov_app *app) {

  if (!app)
    goto error;

  SignalingApp *custom = signaling_app_cast(app->config.userdata);
  if (custom)
    return custom->userdata;
error:
  return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_signaling_app_set_userdata(ov_app *app, void *userdata) {

  if (!app)
    goto error;

  SignalingApp *custom = signaling_app_cast(app->config.userdata);
  if (!custom)
    goto error;

  custom->userdata = userdata;
  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

ov_app *impl_signaling_app_free(ov_app *self) {

  ov_app *app = ov_app_cast(self);
  if (!app)
    goto error;

  SignalingApp *custom = signaling_app_cast(app->config.userdata);
  if (!custom)
    goto error;

  app->free = custom->free;

  // allow subsequent still-in-place callbacks to realize this pointer
  // has been freed
  app->config.userdata = 0;

  // free all custom stuff
  custom->command.functions = ov_dict_free(custom->command.functions);
  custom->command.descriptions =
      ov_json_value_free(custom->command.descriptions);

  free(custom);
  return app->free(app);
error:
  return self;
}

/*----------------------------------------------------------------------------*/

static void invoke_monitor(SignalingApp *custom, ov_direction direction,
                           int socket, const ov_socket_data *remote,
                           ov_json_value const *value) {

  if ((0 != custom) && (0 != custom->monitor)) {
    custom->monitor(custom->monitor_userdata, direction, socket, remote, value);
  }
}

/*----------------------------------------------------------------------------*/

bool ov_signaling_app_io_signaling(ov_app *app, int socket, const char *uuid,
                                   const ov_socket_data *remote,
                                   void **parsed_io_data) {

  if (!app || !remote || !parsed_io_data || !*parsed_io_data)
    goto error;

  ov_log_debug("io signaling at %i|%s from %s:%i", socket, uuid, remote->host,
               remote->port);

  SignalingApp *custom = signaling_app_cast(app->config.userdata);
  if (!custom)
    goto error;

  ov_json_value *value = ov_json_value_cast(*parsed_io_data);
  invoke_monitor(custom, OV_IN, socket, remote, value);

  if (!value) {

    ov_log_error("%s unexpected signaling IO format.", app->config.name);

    goto error;
  }

  /*
   *      Get the event name out of the INPUT.
   */

  ov_json_value const *jstring = ov_json_get(value, "/" OV_EVENT_KEY_EVENT);
  if (!jstring)
    goto error;

  const char *event_key = ov_json_string_get(jstring);
  if (!event_key)
    goto error;

  /*
   *      Get the event handler out of the registered handlers.
   */

  void *val = ov_dict_get(custom->command.functions, event_key);
  if (!val) {
    ov_log_info("%s event |%s| not found for %i|%s", app->config.name,
                event_key, socket, uuid);
    goto done;
  }

  /*
   *      Perform event handling
   */

  ov_json_value *(*callback)(ov_app *, const char *, const ov_json_value *, int,
                             const ov_socket_data *) = val;

  ov_json_value *result = callback(app, event_key, value, socket, remote);

  if (FAILURE_CLOSE_SOCKET == result) {

    ov_log_error("%s event |%s| with result close at "
                 "%i|%s from %s:%i",
                 app->config.name, event_key, socket, uuid, remote->host,
                 remote->port);
    goto error;

  } else {

    ov_log_debug("%s event |%s| at %i|%s from %s:%i", app->config.name,
                 event_key, socket, uuid, remote->host, remote->port);

    if (result) {

      if (!ov_json_value_cast(result)) {
        ov_log_error("%s event |%s| non JSON at %i|%s", app->config.name,
                     event_key, socket, uuid);
        goto error;
      }

      if (!ov_app_send(app, socket, remote, result)) {

        ov_log_error("%s event |%s| send with result "
                     "close socket at %i|%s from %s:%i",
                     app->config.name, event_key, socket, uuid, remote->host,
                     remote->port);

        result = ov_json_value_free(result);
        goto error;
      }

      result = ov_json_value_free(result);
    }
  }

done:
  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

bool ov_signaling_app_register_command(
    ov_app *app, const char *name, const char *description,
    ov_json_value *(*callback)(ov_app *app, const char *name,
                               const ov_json_value *value, int socket,
                               const ov_socket_data *remote)) {

  char *key = NULL;

  if (!ov_app_cast(app))
    goto error;

  SignalingApp *custom = signaling_app_cast(app->config.userdata);
  if (!custom)
    goto error;

  if (!name || !callback) {
    ov_log_error("%s event name or callback missing.", app->config.name);
    goto error;
  }

  key = strdup(name);

  if (!ov_dict_set(custom->command.functions, key, callback, NULL)) {
    ov_log_error("%s failed to set event callback %s", app->config.name, name);
    goto error;
  }

  key = NULL;

  ov_json_value *content = NULL;

  if (description) {

    content = ov_json_string(description);

  } else {

    content = ov_json_null();
  }

  if (!content)
    goto error;

  if (!ov_json_object_set(custom->command.descriptions, name, content)) {
    ov_json_value_free(content);
    goto error;
  }

  return true;
error:
  if (key)
    free(key);
  return false;
}

/*----------------------------------------------------------------------------*/

const ov_json_value *ov_signaling_app_get_commands(const ov_app *app) {

  if (!app)
    goto error;

  SignalingApp *custom = signaling_app_cast(app->config.userdata);
  if (!custom)
    goto error;

  return custom->command.descriptions;
error:
  return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_signaling_app_set_monitor(ov_app *app,
                                  void (*monitor)(void *, ov_direction, int,
                                                  const ov_socket_data *,
                                                  const ov_json_value *),
                                  void *userdata) {

  if (0 == app)
    goto error;

  SignalingApp *sapp = signaling_app_cast(app->config.userdata);
  if (0 == sapp)
    goto error;

  if (0 == monitor)
    monitor = default_monitor;

  sapp->monitor = monitor;
  sapp->monitor_userdata = userdata;

  return true;

error:

  return false;
}

/*----------------------------------------------------------------------------*/

static bool impl_signaling_app_send(ov_app *app, int socket,
                                    const ov_socket_data *remote, void *data) {

  ov_socket_data remote_extracted;
  memset(&remote_extracted, 0, sizeof(remote_extracted));

  ov_socket_data const *remote_to_log = remote;

  if (0 == app)
    goto error;

  SignalingApp *sapp = signaling_app_cast(app->config.userdata);
  if (0 == sapp)
    goto error;

  if (0 == remote_to_log) {

    ov_socket_get_data(socket, 0, &remote_extracted);
    remote_to_log = &remote_extracted;
  }

  invoke_monitor(sapp, OV_OUT, socket, remote_to_log, data);

  return sapp->original_send(app, socket, remote, (void *)data);

error:

  return false;
}

/*----------------------------------------------------------------------------*/

static ov_json_value *cb_help(ov_app *app, const char *name,
                              const ov_json_value *input, int socket,
                              const ov_socket_data *remote) {

  if (!app || !name || !input || !remote || (0 == socket))
    goto error;

  ov_json_value *out = NULL;
  const ov_json_value *commands = ov_signaling_app_get_commands(app);
  if (commands) {
    if (!ov_json_value_copy((void **)&out, commands))
      goto error;
  }

  return out;
error:
  return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_signaling_app_enable_help(ov_app *app) {

  if (!ov_signaling_app_register_command(app, OV_EVENT_HELP,
                                         "get a list of all commands and "
                                         "descriptions",
                                         cb_help))
    goto error;

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool delayed_shutdown(uint32_t timer_id, void *data) {

  OV_ASSERT(data);

  ov_event_loop *loop = ov_event_loop_cast(data);

  if (!loop || !loop->stop)
    goto error;

  loop->stop(loop);

  return true;
error:
  if (timer_id) { /*unused*/
  }
  return false;
}

/*----------------------------------------------------------------------------*/

static ov_json_value *cb_shutdown(ov_app *app, const char *name,
                                  const ov_json_value *input, int socket,
                                  const ov_socket_data *remote) {

  if (!app || !name || !input || !remote || (0 == socket))
    goto error;

  ov_event_loop *loop = app->config.loop;
  if (!loop)
    goto error;

  ov_log_info("%s received %s at %i from %s:%i", app->config.name, name, socket,
              remote->host, remote->port);

  /*
   *      @NOTE "Cosmetic delay"
   *
   *      We delay the shutdown,
   *      and return NULL (do not answer)
   *      to let the app parser clear the input message.
   *
   *      Otherwise we would get some memleaks due to
   *      undeleted input data, which is annoying for debug.
   */

  if (OV_TIMER_INVALID ==
      loop->timer.set(loop, IMPL_SHUTDOWN_DELAY, loop, delayed_shutdown))
    loop->stop(loop);

error:
  return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_signaling_app_enable_shutdown(ov_app *app) {

  if (!ov_signaling_app_register_command(app, OV_KEY_SHUTDOWN,
                                         "Shutdown the eventloop", cb_shutdown))
    goto error;

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

#include <ov_base/ov_parser_json.h>

bool ov_signaling_app_connect(ov_app *self, ov_app_socket_config config) {

  TODO("Serves mainly as doc on how to actually use ov_signaling_app - "
       "Look for better solution and remove");

  if (0 == self) {
    ov_log_error("No app given");
    goto error;
  }

  if (0 == config.callback.io) {
    config.callback.io = ov_signaling_app_io_signaling;
  }

  if (0 == config.parser.create) {
    config.parser.create = ov_parser_json_create_default;
    config.parser.config = (ov_parser_config){.buffering = true};
  }

  return ov_app_connect(self, config);

error:

  return false;
}

/*----------------------------------------------------------------------------*/

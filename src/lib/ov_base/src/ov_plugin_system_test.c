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

#include "ov_plugin_system.c"
#include <ov_test/ov_test.h>

/*----------------------------------------------------------------------------*/

// Test symbol export to plugins

static size_t counter = 0;

static void counter_inc() { ++counter; }

/*----------------------------------------------------------------------------*/

int test_ov_plugin_system_load() {

  testrun(0 == ov_plugin_system_get_plugin(OV_PLUGIN_SYSTEM_TEST_ID));

  char const *invalid_so = OPENVOCS_ROOT "/build/lib/ov_test2.so";

  testrun(0 == ov_plugin_system_load(0, (ov_plugin_load_flags){0}));
  testrun(0 == ov_plugin_system_load(invalid_so, (ov_plugin_load_flags){0}));

  // Now try with proper SO
  // the base library provides the ov_plugin hooks ...
  char const *valid_so = OPENVOCS_ROOT "/build/lib/libov_base2.so";

  void *handle = ov_plugin_system_load(valid_so, (ov_plugin_load_flags){0});
  testrun(0 != handle);

  // Handle should have been registered...
  handle = ov_plugin_system_get_plugin(OV_PLUGIN_SYSTEM_TEST_ID);
  testrun(0 != handle);

  bool (*plugin_was_loaded)(void) =
      ov_plugin_system_get_symbol(handle, "plugin_state");
  testrun(0 != plugin_was_loaded);

  testrun(plugin_was_loaded());

  testrun(ov_plugin_system_close());

  /*************************************************************************
                       Same again without registering
   ************************************************************************/

  handle =
      ov_plugin_system_load(valid_so, (ov_plugin_load_flags){.simple = true});
  testrun(0 != handle);

  // Handle should have not been registered...
  testrun(0 == ov_plugin_system_get_plugin(OV_PLUGIN_SYSTEM_TEST_ID));

  plugin_was_loaded = ov_plugin_system_get_symbol(handle, "plugin_state");
  testrun(0 != plugin_was_loaded);

  testrun(plugin_was_loaded());

  testrun(ov_plugin_system_close());

  /*************************************************************************
                       Test symbol export to plugins
   ************************************************************************/

  testrun(ov_plugin_system_export_symbol_for_plugin("counter_1", counter_inc));

  handle = ov_plugin_system_load(valid_so, (ov_plugin_load_flags){0});
  testrun(0 != handle);

  plugin_was_loaded = ov_plugin_system_get_symbol(handle, "plugin_state");
  testrun(0 != plugin_was_loaded);

  testrun(plugin_was_loaded());

  testrun(ov_plugin_system_close());

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_plugin_system_load_dir() {

  testrun(0 == ov_plugin_system_get_plugin(OV_PLUGIN_SYSTEM_TEST_ID));

  // Result depends on wether there are plugins under
  // /usr/lib/openvocs/plugins
  ov_plugin_system_load_dir(0);
  testrun(0 == ov_plugin_system_get_plugin(OV_PLUGIN_SYSTEM_TEST_ID));

  testrun(0 < ov_plugin_system_load_dir(OPENVOCS_ROOT "/build/lib"));

  void *handle = ov_plugin_system_get_plugin(OV_PLUGIN_SYSTEM_TEST_ID);
  testrun(0 != handle);

  bool (*plugin_was_loaded)(void) =
      ov_plugin_system_get_symbol(handle, "plugin_state");
  testrun(0 != plugin_was_loaded);

  // testrun(plugin_was_loaded());

  testrun(ov_plugin_system_close());

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_plugin_system", test_ov_plugin_system_load,
            test_ov_plugin_system_load_dir);

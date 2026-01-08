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

        Will only start SNMP agent if the plugin_snmp plugin is installed.

        Then BEWARE:

        REQUIRES RUNNING SNMPD daemon with
        'master xagent' in snmpd.conf

        REQUIRES START AS ROOT

        If it does not work:

        Agent can connect to master, but oid is reported as "No such ...":

            - Check that snmpd.conf does not restrict views, e.g. via
              'rocommunity public default -V systemctonly'
              If there, change to
              'rocommunity public default'


        ------------------------------------------------------------------------
*/

#include "../include/ov_snmp.h"
#include <ov_log/ov_log.h>

bool ov_snmp_agent_running() {

  bool (*agent_running)(void) = ov_plugin_system_get_symbol(
      ov_plugin_system_get_plugin(OV_PLUGIN_SNMP_ID),
      "ov_plugin_snmp_agent_running");

  if (agent_running) {
    return agent_running();
  } else {
    return false;
  }
}

/*----------------------------------------------------------------------------*/

void ov_snmp_agent_start() {

  void (*agent_start)(void) = ov_plugin_system_get_symbol(
      ov_plugin_system_get_plugin(OV_PLUGIN_SNMP_ID),
      "ov_plugin_snmp_agent_start");

  if (agent_start) {
    agent_start();
    ov_log_info("Started SNMP Agent");
  } else {
    ov_log_warning("Could not start SNMP Agent");
  }
}

/*----------------------------------------------------------------------------*/

void ov_snmp_agent_stop() {

  void (*agent_stop)(void) = ov_plugin_system_get_symbol(
      ov_plugin_system_get_plugin(OV_PLUGIN_SNMP_ID),
      "ov_plugin_snmp_agent_stop");

  if (agent_stop) {
    agent_stop();
  }
}

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

        @author Michael J. Beer, DLR/GSOC
        @copyright (c) 2023 German Aerospace Center DLR e.V. (GSOC)

        For an easy doc of SIP transactions:

        http://www.siptutorial.net/SIP

        ------------------------------------------------------------------------
*/
#ifndef OV_SNMP_H
#define OV_SNMP_H
/*----------------------------------------------------------------------------*/

#include <ov_base/ov_plugin_system.h>

/*----------------------------------------------------------------------------*/

#define OV_PLUGIN_SNMP_ID "snmp"

bool ov_snmp_agent_running();

void ov_snmp_agent_start();

void ov_snmp_agent_stop();

/*----------------------------------------------------------------------------*/
#endif

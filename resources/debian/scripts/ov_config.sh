#!/usr/bin/env bash
#
#       Copyright (c) 2023 German Aerospace Center DLR e.V. (GSOC)
#
#       Licensed under the Apache License, Version 2.0 (the "License");
#       you may not use this file except in compliance with the License.
#       You may obtain a copy of the License at
#
#               http://www.apache.org/licenses/LICENSE-2.0
#
#       Unless required by applicable law or agreed to in writing, software
#       distributed under the License is distributed on an "AS IS" BASIS,
#       WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#       See the License for the specific language governing permissions and
#       limitations under the License.
#
#       This file is part of the openvocs project. http://openvocs.org
#
#       ------------------------------------------------------------------------
#
#       Authors         Markus Toepfer, Michael Beer
#
#       ------------------------------------------------------------------------

function setup_sip_gateway() {

    echo "{
        \"sip_gateway\": {
            \"resource_manager\": {
                \"host\": \"$SIP_HOST\",
                \"port\": $SIP_PORT,
                \"type\": \"TCP\"
            },
            \"sip_server\": {
                \"host\": \"$SIP_ASTERISK_HOST\",
                \"port\": 5060,
                \"type\": \"TCP\"
            },
            \"signaling\": {
                \"host\": \"127.0.0.1\",
                \"port\": 20001,
                \"type\": \"TCP\"
            },
            \"log\": {
                \"level\": \"debug\",
                \"systemd\": false,
                \"file\": \"/tmp/ov_sip_gateway.log\"
            },
            \"reconnect_interval_secs\": 5,
            \"local_sip_tcp_socket\": {
                \"host\": \"$IP\",
                \"port\": 5060,
                \"type\": \"TCP\"
            }
        }
    }" > $DIR_OV_MC_SIP/config.json

}

function generate_config_recorder() {

    echo "{
        \"recorder\": {
            \"signalling_server\": {
                \"host\": \"127.0.0.1\",
                \"port\": 10001,
                \"type\": \"TCP\",
                \"reconnect_interval_secs\": 10
            },
            \"repository_root_path\": \"/mnt/openvocs/recordings\",
            \"log\": {
                \"level\": \"debug\",
                \"systemd\": true
            },
            \"cache\": {
                \"enabled\": true,
                \"sizes\": {
                    \"buffers\": 27,
                    \"lists\": 27,
                    \"rtp_frame\": 27,
                    \"values\": 27,
                    \"frame_data\": 27,
                    \"frame_data_list\": 27,
                    \"frame_data_message\": 27,
                    \"ov_rtp_frame_message\": 27,
                    \"ov_rtp_list_message\": 27,
                    \"mixed_data_message\": 27
                }
            },
            \"lock_timeout_msecs\": 1000,
            \"vad\": {
                \"zero_crossings_rate_hertz\": 10000,
                \"powerlevel_density_dbfs\": -50,
                \"silence_cutoff_interval_secs\": 10
            }
        }
    }" > $DIR_OV_MC_RECORDER/config.json

}

if [ "$EUID" -ne 0 ]
  then echo "Please run as root"
  exit
fi


if [[ -z "$1" ]]; then
    echo "You need to provide some IP/HOST to generate the config."
    exit
fi

SIP_ASTERISK_HOST=""
if [ "X" != "X$2" ]; then
    SIP_ASTERISK_HOST=$2
fi

CWD=$PWD

IP=$1

DIR_HTML="/srv/openvocs/HTML"
DIR_CONFIG="/etc/openvocs"

DIR_OV_MC_VOCS=$DIR_CONFIG/ov_mc_vocs
DIR_OV_MC_ICE_PROXY=$DIR_CONFIG/ov_mc_ice_proxy
DIR_OV_MC_MIXER=$DIR_CONFIG/ov_mc_mixer
DIR_OV_MC_VAD=$DIR_CONFIG/ov_mc_vad
DIR_OV_MC_SIP=$DIR_CONFIG/ov_mc_sip
DIR_OV_MC_RECORDER=$DIR_CONFIG/ov_mc_recorder

DIR_DOMAINS=$DIR_OV_MC_VOCS/domains
DIR_MIME=$DIR_OV_MC_VOCS/mime
DIR_AUTH=$DIR_OV_MC_VOCS/auth

###############################################################################
#                                   Sockets
###############################################################################

ICE_INTERNAL_HOST="127.0.0.1"

ICE_PROXY_HOST=$ICE_INTERNAL_HOST
ICE_PROXY_PORT=30000
EVENTS_PORT=30002

ICE_PROXY_EXTERNAL_HOST=$IP
ICE_PROXY_EXTERNAL_PORT=30001

MIXER_HOST=$ICE_INTERNAL_HOST
MIXER_PORT=40000

VAD_HOST=$ICE_INTERNAL_HOST
VAD_PORT=40001

SIP_HOST=$IP
SIP_PORT=12344
SIP_STATIC_PORT=12343

#-------------------------------------------------------------------------------

function generate_config_domain() {

echo "{
   \"name\":\"$IP\",
   \"path\":\"$DIR_HTML\",
   \"certificate\": {
    \"file\" : \"$DIR_OV_MC_VOCS/$IP.crt\",
    \"key\": \"$DIR_OV_MC_VOCS/$IP.key\"
  }
}" > $DIR_DOMAINS"/domain.json"
}

#-------------------------------------------------------------------------------

generate_config_ice_proxy() {

   echo "{
     \"log\" : {
       \"systemd\" : true,
       \"file\" : \"stdout\",
       \"level\" : \"debug\"
     },
     \"proxy\" :
     {
       \"multiplexing\" : true,
       \"dynamic\" :
       {
        \"port\" :
        {
         \"min\" : 1025,
         \"max\" : 65535
        },
       \"stun\" : 
       {
        \"server\" : 
        {
          \"host\" : \"87.238.197.166\",
          \"type\" : \"UDP\",
          \"port\" : 3478
        }
       },
       \"turn\" : 
       {
        \"server\" : 
        {
          \"host\" : \"127.0.0.1\",
          \"type\" : \"UDP\",
          \"port\" : 12345
         },
        \"user\" : \"user1\",
        \"password\" : \"user1\"
       }
       },
       \"ssl\" :
       {
         \"certificate\" : \"$DIR_OV_MC_VOCS/$IP.crt\",
         \"key\" :  \"$DIR_OV_MC_VOCS/$IP.key\",
         \"CA file\" : \"$DIR_OV_MC_VOCS/$IP.crt\"
       },
       \"manager\" :
       {
         \"host\" : \"$ICE_PROXY_HOST\",
         \"port\" : $ICE_PROXY_PORT,
         \"type\" : \"TCP\"
       },
       \"external\" :
       {
            \"host\" : \"$ICE_PROXY_EXTERNAL_HOST\",
            \"port\" : $ICE_PROXY_EXTERNAL_PORT,
            \"type\" : \"UDP\"
       },
       \"internal\" :
       {
            \"host\" : \"$ICE_PROXY_EXTERNAL_HOST\",
            \"port\" : 0,
            \"type\" : \"UDP\"
       },
       \"limits\":
       {
          \"num_treads\" : 0,
          \"message_queue_capacity\": 0,
          \"threadlock timeout (usec)\": 0,
          \"sessions\": 0,
          \"client\" : 4,
          \"accept timeout (usec)\": 1000000,
          \"timeout (usec)\" : 0,
          \"reconnect timeout (usec)\": 2000000,
          \"transaction lifetime (usec)\": 15000000
       }
     }
   }" > $DIR_OV_MC_ICE_PROXY"/config.json"

}

#-------------------------------------------------------------------------------

generate_config_mixers() {

   echo "{
     \"log\" : {
       \"systemd\" : true,
       \"file\" : \"stdout\",
       \"level\" : \"debug\"
     },
     \"app\" :
     {
       \"resource_manager\" :
       {
         \"host\" : \"$MIXER_HOST\",
         \"port\" : $MIXER_PORT,
         \"type\" : \"TCP\"
       },
       \"limit\":
       {
         \"reconnect_interval_secs\" : 3
       }
     }
   }" > $DIR_OV_MC_MIXER"/config.json"

}

#-------------------------------------------------------------------------------

generate_config_vad() {

   echo "{
     \"log\" : {
       \"systemd\" : true,
       \"file\" : \"stdout\",
       \"level\" : \"debug\"
     },
     \"vad\" :
     {
       \"socket\" :
       {
         \"host\" : \"$VAD_HOST\",
         \"port\" : $VAD_PORT,
         \"type\" : \"TCP\"
       },
       \"vad\":
       {
         \"zero_crossings_rate_hertz\" : 40000,
         \"powerlevel_density_dbfs\": -340
       }
     }
   }" > $DIR_OV_MC_VAD"/config.json"

}


#-------------------------------------------------------------------------------

generate_config_ov_vocs() {

   echo "{
     \"log\" : {
       \"systemd\" : true,
       \"file\" : \"stdout\",
       \"level\" : \"debug\"
     },
     \"vocs\" :
     {
       \"domain\" : \"$IP\",

       \"sip\" :
       {
         \"timeout\":
         {
           \"response timeout (usec)\" : 20000000
         },
         \"socket\" :
         {
           \"manager\" :
           {
             \"host\" : \"$SIP_HOST\",
             \"type\" : \"TCP\",
             \"port\" : $SIP_PORT
           }
         }
       },
       \"sip static\" :
       {
         \"socket\" :
         {
           \"manager\" :
           {
             \"host\" : \"$SIP_HOST\",
             \"type\" : \"TCP\",
             \"port\" : $SIP_STATIC_PORT
           }
         }
       },
       \"frontend\" :
       {
         \"socket\" :
         {
           \"manager\":
           {
             \"host\" : \"$ICE_PROXY_HOST\",
             \"port\" : $ICE_PROXY_PORT,
             \"type\" : \"TCP\"
           }
         }
       },
       \"events\" :
       {
         \"socket\" :
         {
           \"manager\":
           {
             \"host\" : \"$ICE_INTERNAL_HOST\",
             \"port\" : $EVENTS_PORT,
             \"type\" : \"TCP\"
           }
         }
       },
       \"vad\" :
         {
            \"socket\" :
            {
            \"host\" : \"$VAD_HOST\",
             \"port\" : $VAD_PORT,
             \"type\" : \"TCP\"
            }
         },
       \"backend\" :
       {
         \"socket\" :
         {
           \"manager\":
           {
             \"host\" : \"$MIXER_HOST\",
             \"port\" : $MIXER_PORT,
             \"type\" : \"TCP\"
           }
         },
         
         \"mixer\":
         {
           \"vad\" :
           {
             \"zero_crossings_rate_hertz\" : 50000,
             \"powerlevel_density_dbfs\" : -500,
             \"enabled\" : true
           },
           \"sample_rate_hz\" : 48000,
           \"noise\" : -70,
           \"max_num_frames\" : 100,
           \"frame_buffer\": 1024,
           \"normalize_input\" : false,
           \"rtp_keepalive\" : true,
           \"normalize_mixed_by_root\" : false
         }
       },
       \"recorder\" :
       {
          \"socket\":
          {
            \"manager\" :
            {
              \"host\" : \"127.0.0.1\",
              \"type\" : \"TCP\",
              \"port\" : 10001
            }
          },
          \"db\" :
          {
             \"type\" : \"sqlite\",
             \"db\" : \"/tmp/openvocs.sqlite3\"
          }
       }
     },
     \"db\" :
     {
       \"git\" :false,
       \"path\" : \"$DIR_OV_MC_VOCS\",
       \"timeout\" :
       {
         \"ldap\" : 5000000,
         \"threadlock timeout (usec)\": 1000000,
         \"state snapshot (sec)\" : 60,
         \"auth snapshot (sec)\" : 300
       },
       \"password\" :
       {
         \"length\" : 32
       }
     },
     \"ldap\" : {
       \"enabled\" : false,
       \"threads\" : 4,
       \"host\": \"localhost\",
       \"user_dn_tree\" : \"ou=people,dc=openvocs,dc=org\",
       \"timeout\":
       {
         \"network\" : 3000000
       }
     },
     \"webserver\":
     {
       \"name\":\"VOCS GATEWAY\",
       \"debug\":false,
       \"ip4_only\":true,
       \"domains\":\"$DIR_DOMAINS\",
       \"mime\" : {
         \"path\" : \"$DIR_MIME\",
         \"extension\" : \"mime\"
       },
       \"sockets\":
       {
         \"https\":
         {
           \"host\":\"$IP\",
           \"port\":443,
           \"type\":\"TCP\"
         },
         \"stun\":
         [
           {
             \"host\":\"$IP\",
             \"port\":3478,
             \"type\":\"UDP\"
           }
         ]
       }
     }
   }" > $DIR_OV_MC_VOCS"/config.json"

}

#-------------------------------------------------------------------------------

function generate_certificates() {

   echo "#
   # Standart OpenSSL configuration file to generate the ov.test
   # certificates
   #
   
   [ req ]
   
   default_bits    = 2048
   distinguished_name  = req_distinguished_name
   req_extensions    = req_ext
   x509_extensions = v3_req
   
   prompt        = no
   
   [ req_distinguished_name ]
   
   countryName     = DE
   stateOrProvinceName = Berlin
   localityName    = Berlin
   organizationName  = DLR e.V.
   organizationalUnitName=openvocs
   commonName      = $IP self signed for ov.test
   emailAddress    = dlr@openvocs.de
   
   [ req_ext ]
   
   subjectAltName = @alt_names[v3_req]
   subjectAltName=@alt_names
   
   [ alt_names ]
   
   IP.1 = $IP
   IP.2 = 127.0.0.1" > $DIR_OV_MC_VOCS"/ssl.cnf"
   
   NAME=$IP
   CONF=$DIR_OV_MC_VOCS"/ssl.cnf"
   DAYS=365
   RSA=4096
   
   openssl req -x509 -newkey $RSA -nodes -keyout $NAME.key -days $DAYS -out $NAME.crt -extensions req_ext -config $CONF
   
   #       ------------------------------------------------------------------------
   #       CHANGE MODESET FOR TESTS
   #       ------------------------------------------------------------------------
   
   chmod a+r $NAME.crt
   chmod a+r $NAME.key

}

###############################################################################
#                                     MAIN
###############################################################################


echo "start creating config at: $DIR_OV_MC_VOCS"
echo ""

cd $DIR_OV_MC_VOCS

if [ "X" != "X$SIP_ASTERISK_HOST" ]; then
    # Asterisk requires to be able to exchange UDP data with the mixers,
    # hence the mixers must listen on the external interface
    ICE_PROXY_HOST="$IP"
    MIXER_HOST="$IP"
    setup_sip_gateway
fi


generate_config_domain
generate_config_ice_proxy
generate_config_mixers
generate_config_ov_vocs
generate_config_vad
generate_certificates
generate_config_recorder

cd $CWD

echo ""
echo "!!! NOTE !!!"
echo ""
echo "The generated certificate key file is readable by anyone."
echo "(1) This is desired for test runs."
echo "(2) This is NOGO FOR ALL OPERATIONAL SCENARIOS."
echo ""

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
        @file           ov_config_keys.h
        @author         Markus Toepfer

        @date           2019-06-05

        @ingroup        ov_config_keys

        @brief          Definition of ov_config_keys.

                        This is a base set of keys using for JSON structures
                        or configurations in ov_*


        ------------------------------------------------------------------------
*/
#ifndef ov_config_keys_h
#define ov_config_keys_h

/*
 *      ------------------------------------------------------------------------
 *
 *      API KEY DEFINITIONS
 *
 *      ------------------------------------------------------------------------
 */

#define OV_KEY_API_VOICE_CLIENT "voice client"
#define OV_KEY_API_ICE_PROXY_MANAGER "ice proxy manager"
#define OV_KEY_API_RESOURCE_MANAGER "resource manager"
#define OV_KEY_API_ICE_PROXY_API "ice proxy"
#define OV_KEY_RECORDER "recorder"
#define OV_KEY_RECORDINGS "recordings"
#define OV_KEY_META "meta"

/*
 *      ------------------------------------------------------------------------
 *
 *      MAIN KEY DEFINITIONS
 *
 *      ------------------------------------------------------------------------
 */

#define OV_KEY_CLIENT_API_TEST "client_api_test"

#define OV_KEY_PROCESSING "processing"
#define OV_KEY_EMPTY "empty"
#define OV_KEY_DEFAULTS "defaults"
#define OV_KEY_ANY "any"

#define OV_KEY_EVENTS "events"
#define OV_KEY_EVENT "event"
#define OV_KEY_UUID "uuid"
#define OV_KEY_TYPE "type"
#define OV_KEY_CLASS "class"
#define OV_KEY_PROTOCOL "protocol"
#define OV_KEY_ID "id"
#define OV_KEY_IO "io"
#define OV_KEY_VERSION "version"
#define OV_KEY_AVATAR "avatar"
#define OV_KEY_LOGO "logo"
#define OV_KEY_NOTE "note"
#define OV_KEY_AGENT "agent"
#define OV_KEY_MANAGER "manager"

#define OV_KEY_LDAP "ldap"
#define OV_KEY_DB "db"
#define OV_KEY_CA "ca"
#define OV_KEY_CA_FILE "CA file"
#define OV_KEY_CA_PATH "CA path"
#define OV_KEY_MULTICAST "multicast"

#define OV_KEY_INPUT "input"
#define OV_KEY_OUTPUT "output"

#define OV_KEY_APP "app"
#define OV_KEY_API "api"
#define OV_KEY_SIP "sip"
#define OV_KEY_SIP_STATIC "sip static"

#define OV_KEY_LIMITS "limits"
#define OV_KEY_LIMIT "limit"
#define OV_KEY_UNLIMITED "unlimited"
#define OV_KEY_CAPACITY "capacity"
#define OV_KEY_CALLBACK "callback"
#define OV_KEY_USERDATA "userdata"
#define OV_KEY_RECONNECT "reconnect"
#define OV_KEY_RECONNECT_USEC "reconnect (usec)"

#define OV_KEY_CONNECT "connect"
#define OV_KEY_CONNECTED "connected"

#define OV_KEY_SECONDS "seconds"
#define OV_KEY_MILLISECONDS "milliseconds"
#define OV_KEY_MICROSECONDS "microseconds"

#define OV_KEY_OWNER "owner"
#define OV_KEY_ADMIN "admin"

#define OV_KEY_FROM "from"
#define OV_KEY_TO "to"

#define OV_KEY_JOIN "join"
#define OV_KEY_LEAVE "leave"

#define OV_KEY_RELEASE "release"
#define OV_KEY_ACQUIRE "acquire"

#define OV_KEY_LOGIN "login"
#define OV_KEY_AUTHENTICATE_PASSWORD "authenticate password"
#define OV_KEY_AUTHORISE "authorise"
#define OV_KEY_AUTHORIZE "authorize"

#define OV_KEY_LOGOUT "logout"

#define OV_KEY_CONFIG "config"
#define OV_KEY_CONFIGURATION OV_KEY_CONFIG
#define OV_KEY_CONFIGURE "configure"

#define OV_KEY_PORT "port"
#define OV_KEY_HOST "host"
#define OV_KEY_NETWORK "network"

#define OV_KEY_ASYNC "async"
#define OV_KEY_SYNC "sync"

#define OV_KEY_BACKEND "backend"
#define OV_KEY_FRONTEND "frontend"
#define OV_KEY_ENDPOINT "endpoint"

#define OV_KEY_FORWARD "forward"

#define OV_KEY_REMOTE "remote"
#define OV_KEY_LOCAL "local"
#define OV_KEY_PROXY "proxy"
#define OV_KEY_ACCEPT "accept"
#define OV_KEY_ACCEPT_TIMEOUT_USEC "accept timeout (usec)"
#define OV_KEY_TRIGGER_TIMEOUT_USEC "trigger timeout (usec)"
#define OV_KEY_DEFAULT "default"

#define OV_KEY_MESSAGE "message"

#define OV_KEY_ICE "ice"

#define OV_KEY_NAME "name"
#define OV_KEY_COLOR "color"
#define OV_KEY_RATE "rate"
#define OV_KEY_CODE "code"
#define OV_KEY_HELP "help"
#define OV_KEY_STATE "state"
#define OV_KEY_STATUS "status"
#define OV_KEY_TRIGGER "trigger"
#define OV_KEY_CLOSE "close"

#define OV_KEY_AUTH "auth"
#define OV_KEY_AUTHENTICATION "authentication"
#define OV_KEY_AUTHORIZATION "authorisation"
#define OV_KEY_ACCOUNTING "accounting"

#define OV_KEY_DESC "description"
#define OV_KEY_DESCRIPTION OV_KEY_DESC
#define OV_KEY_EXT "extensions"
#define OV_KEY_EXTENTIONS OV_KEY_EXT
#define OV_KEY_PARAMS "parameter"
#define OV_KEY_PARAMETER OV_KEY_PARAMS
#define OV_KEY_COMP "component"
#define OV_KEY_COMPONENT OV_KEY_COMP
#define OV_KEY_PRIO "priority"
#define OV_KEY_PRIORITY OV_KEY_PRIO
#define OV_KEY_ATTRIBUTES "attributes"

#define OV_KEY_SOFTWARE "software"
#define OV_KEY_TRANSPORT "transport"
#define OV_KEY_REALM "realm"
#define OV_KEY_RELAY "relay"
#define OV_KEY_THREAD "thread"
#define OV_KEY_THREADS "threads"

#define OV_KEY_LIST "list"
#define OV_KEY_DICT "dict"
#define OV_KEY_ITEM "item"
#define OV_KEY_PAIR "pair"
#define OV_KEY_KEY "key"
#define OV_KEY_VALUE "value"

#define OV_KEY_CALL_ID "call-id"
#define OV_KEY_PAYLOAD "payload"
#define OV_KEY_TAG "tag"
#define OV_KEY_DIALOG "dialog"
#define OV_KEY_DIALOGS "dialogs"

#define OV_KEY_SCHEME "scheme"
#define OV_KEY_QUERY "query"
#define OV_KEY_FRAGMENT "fragment"

#define OV_KEY_TIME "time"
#define OV_KEY_URI "uri"
#define OV_KEY_GIT "git"

#define OV_KEY_NEXT "next"
#define OV_KEY_PREV "prev"

#define OV_KEY_TIMER "timer"
#define OV_KEY_TIMEOUT "timeout"
#define OV_KEY_TIMEOUT_SEC "timeout (sec)"
#define OV_KEY_TIMEOUT_USEC "timeout (usec)"
#define OV_KEY_RETRY "retry"
#define OV_KEY_KEEPALIVE "keepalive"
#define OV_KEY_KEEPALIVE_SEC "keepalive (sec)"
#define OV_KEY_LIFETIME "lifetime"
#define OV_KEY_GATHERING "gathering"
#define OV_KEY_GATHERING_USEC "gathering (usec)"

#define OV_KEY_MANAGEMENT "management"

#define OV_KEY_RETRANSMIT "retransmit (usec)"
#define OV_KEY_MAX_LIFETIME "lifetime (usec)"
#define OV_KEY_MAX_KEEPALIVE "keepalive (usec)"
#define OV_KEY_MAX_INACTIVITY "inactivity (usec)"
#define OV_KEY_MAX_PROGRESS_WAIT "progress wait (usec)"
#define OV_KEY_MAX_LOGIN_TIMEOUT "login timeout (usec)"
#define OV_KEY_MAX_BYTES_PER_CYCLE "max bytes per cycle"
#define OV_KEY_MAX_BYTES_PER_WEBSOCKET_FRAME "max bytes per frame"
#define OV_KEY_AUTO_DISCOVERY "autodiscovery"
#define OV_KEY_RECONNECT_TIMEOUT "reconnect timeout (usec)"
#define OV_KEY_RECONNECT_TIMEOUT_USEC OV_KEY_RECONNECT_TIMEOUT
#define OV_KEY_ANSWER_TIMEOUT "answer timeout (usec)"
#define OV_KEY_THREAD_LOCK_TIMEOUT "threadlock timeout (usec)"
#define OV_KEY_RESPONSE_TIMEOUT "response timeout (usec)"
#define OV_KEY_STATE_SNAPSHOT_TIMEOUT "state snapshot (sec)"
#define OV_KEY_AUTH_SNAPSHOT_TIMEOUT "auth snapshot (sec)"
#define OV_KEY_LDAP_TIMEOUT "ldap (sec)"

#define OV_KEY_CONNECTIVITY_CHECK "connectivity check"

#define OV_KEY_FOUNDATION "foundation"
#define OV_KEY_CONTROL "control"
#define OV_KEY_CONTROLING "controling"

#define OV_KEY_IN "in"
#define OV_KEY_OUT "out"

#define OV_KEY_SEQUENCE_NUMBER "sequence_number"
#define OV_KEY_TIMESTAMP "timestamp"

#define OV_KEY_INCOMING "incoming"
#define OV_KEY_OUTGOING "outgoing"

#define OV_KEY_INTERCONNECT "interconnect"
#define OV_KEY_INTERNAL "internal"
#define OV_KEY_EXTERNAL "external"
#define OV_KEY_SECURE "secure"
#define OV_KEY_BASE "base"

#define OV_KEY_REQUEST "request"
#define OV_KEY_RESPONSE "response"
#define OV_KEY_RESULT "result"

#define OV_KEY_CACHE "cache"
#define OV_KEY_SETUP "setup"

#define OV_KEY_CREATE "create"
#define OV_KEY_CREATED "created"
#define OV_KEY_UPDATE "update"
#define OV_KEY_UPDATED "updated"
#define OV_KEY_DELETE "delete"
#define OV_KEY_DELETED "deleted"

#define OV_KEY_USE "use"
#define OV_KEY_USED "used"

#define OV_KEY_SAVE "save"
#define OV_KEY_LOAD "load"
#define OV_KEY_PATH "path"

#define OV_KEY_PUBLIC "public"
#define OV_KEY_PRIVATE "private"
#define OV_KEY_SCOPE "scope"

#define OV_KEY_DETAIL "detail"

#define OV_KEY_OFFER "offer"
#define OV_KEY_ANSWER "answer"

#define OV_KEY_STATISTICS "statistics"
#define OV_KEY_LAST "last"
#define OV_KEY_BYTES "bytes"

#define OV_KEY_IO "io"
#define OV_KEY_TIMING "timing"
#define OV_KEY_SIGNALING "signaling"

#define OV_KEY_LIST_CALLS "list_calls"
#define OV_KEY_LIST_CALL_PERMISSIONS "list_call_permissions"
#define OV_KEY_LIST_PERMISSIONS "list_permissions"
#define OV_KEY_GET_STATUS "get_status"

#define OV_KEY_LIST_SIP_STATUS "list_sip_status"

#define OV_KEY_KEYS "keys"
#define OV_KEY_QUANTITY "quantiy"

#define OV_KEY_MIXER_AQUIRED "mixer aquired"
#define OV_KEY_MIXER_RELEASED "mixer released"
#define OV_KEY_MIXER_LOST "mixer lost"

#define OV_KEY_CLIENT_LOGIN "client login"
#define OV_KEY_CLIENT_LOGOUT "client logout"

#define OV_KEY_SESSION_CREATED "session created"
#define OV_KEY_SESSION_CLOSED "session closed"

#define OV_KEY_WHITELIST "whitelist"
#define OV_KEY_BLACKLIST "blacklist"

#define OV_KEY_LAST_UPDATE "last update"

/*
 *      ------------------------------------------------------------------------
 *
 *      TYPE DEFINITIONS
 *
 *      ------------------------------------------------------------------------
 */

#define OV_KEY_SERVERS "servers"
#define OV_KEY_SERVER "server"

#define OV_KEY_CLIENTS "clients"
#define OV_KEY_CLIENT "client"

#define OV_KEY_CONNECTIONS "connections"
#define OV_KEY_CONNECTION "connection"
#define OV_KEY_LISTENER "listener"

#define OV_KEY_TRANSACTIONS "transactions"
#define OV_KEY_TRANSACTION "transaction"

#define OV_KEY_SESSIONS "sessions"
#define OV_KEY_SESSION "session"

#define OV_KEY_STREAMS "streams"
#define OV_KEY_STREAM "stream"

#define OV_KEY_PAIRS "pairs"
#define OV_KEY_PAIR "pair"

#define OV_KEY_SOCKETS "sockets"
#define OV_KEY_SOCKET "socket"

#define OV_KEY_BASES "bases"
#define OV_KEY_BASE "base"

#define OV_KEY_CANDIDATES "candidates"
#define OV_KEY_CANDIDATE "candidate"
#define OV_KEY_END_OF_CANDIDATES "end_of_candidates"

#define OV_KEY_ENTITY "entity"
#define OV_KEY_MINIONS "minions"

#define OV_KEY_MIXER "mixer"
#define OV_KEY_MULTIPLEXER "multiplexer"
#define OV_KEY_MULTIPLEXING "multiplexing"
#define OV_KEY_DYNAMIC "dynamic"
#define OV_KEY_RECORDER "recorder"
#define OV_KEY_RECORDED "recorded"

#define OV_KEY_ROLL_AFTER_SECS "roll_after_secs"

#define OV_KEY_INTERFACES "interfaces"
#define OV_KEY_INTERFACE "interface"

#define OV_KEY_GROUP "group"
#define OV_KEY_GROUPS "groups"

#define OV_KEY_AUTHENTICATION_REQUIRED "authentication required"
#define OV_KEY_UNAUTHENTICATED "unauthenticated"
#define OV_KEY_AUTHENTICATED "authenticated"
#define OV_KEY_AUTHORIZED "authorized"

#define OV_KEY_REGISTRAR "registrar"
#define OV_KEY_REGISTER "register"
#define OV_KEY_UNREGISTER "unregister"

#define OV_KEY_PERMIT "permit"
#define OV_KEY_REVOKE "revoke"

#define OV_KEY_PERMIT_CALL "permit_call"
#define OV_KEY_REVOKE_CALL "revoke_call"

#define OV_KEY_CLOSE_ON_ERROR "close_on_error"
#define OV_KEY_CLOSE_ON_LOGOUT "close_on_logout"

#define OV_KEY_FORCE_INTERFACES "force_interfaces"
#define OV_KEY_DROP_CALLS_ON_RESMGR_LOSS "drop_calls_on_resmgr_loss"

#define OV_KEY_NAT_IP "nat_ip"

/*
 *      ------------------------------------------------------------------------
 *
 *      SERVICES
 *
 *      ------------------------------------------------------------------------
 */

#define OV_KEY_RESOURCE_MANAGER "resource_manager"
#define OV_KEY_CHANNELS "channels"
#define OV_KEY_INPUT "input"
#define OV_KEY_OUTPUT "output"
#define OV_KEY_ALSA "alsa"
#define OV_KEY_SIP_SERVER "sip_server"
#define OV_KEY_SIP_SERVER "sip_server"
#define OV_KEY_LOCAL_SIP_TCP_SOCKET "local_sip_tcp_socket"
#define OV_KEY_WEBGATEWAY "webgateway"
#define OV_KEY_ANONYMITY "anonymity"
#define OV_KEY_SHA256 "sha256"

#define OV_KEY_HASH "hash"
#define OV_KEY_SALT "salt"

#define OV_KEY_WORKFACTOR "workfactor"
#define OV_KEY_BLOCKSIZE "blocksize"
#define OV_KEY_PARALLEL "parallel"

/*
 *      ------------------------------------------------------------------------
 *
 *      LOG AND INFO LEVEL KEYS
 *
 *      ------------------------------------------------------------------------
 */

#define OV_KEY_EMERGENCY "emergency"
#define OV_KEY_ALERT "alert"
#define OV_KEY_CRITICAL "critical"
#define OV_KEY_WARNING "warning"
#define OV_KEY_NOTICE "notice"

#define OV_KEY_ERROR "error"
#define OV_KEY_ERRORS "errors"
#define OV_KEY_SUCCESS "success"
#define OV_KEY_ANSWER "answer"
#define OV_KEY_ANSWER_CLOSE "answer close"

#define OV_KEY_DEBUG "debug"
#define OV_KEY_INFO "info"
#define OV_KEY_FAILURE "failure"

#define OV_KEY_PROGRESS "progress"
#define OV_KEY_MISMATCH "mismatch"

#define OV_KEY_PRESENCE "presence"
#define OV_KEY_FINGERPRINT "fingerprint"

#define OV_KEY_ACQUIRED "acquired"

/*
 *      ------------------------------------------------------------------------
 *
 *      DISTRIBUTION TYPES
 *
 *      ------------------------------------------------------------------------
 */

#define OV_KEY_UNICAST "unicast"
#define OV_KEY_MULTICAST "multicast"
#define OV_KEY_BROADCAST "broadcast"
#define OV_KEY_USER_BROADCAST "user_broadcast"
#define OV_KEY_ROLE_BROADCAST "role_broadcast"
#define OV_KEY_LOOP_BROADCAST "loop_broadcast"
#define OV_KEY_CHANNEL_BROADCAST "channel_broadcast"

/*
 *      ------------------------------------------------------------------------
 *
 *      PROTOCOL TYPES
 *
 *      ------------------------------------------------------------------------
 */

#define OV_KEY_TCP "tcp"
#define OV_KEY_UDP "udp"
#define OV_KEY_DTLS "dtls"
#define OV_KEY_STUN "stun"
#define OV_KEY_TURN "turn"
#define OV_KEY_SSL "ssl"
#define OV_KEY_TLS "tls"
#define OV_KEY_LOCAL "local"

#define OV_KEY_SDP "sdp"
#define OV_KEY_JSON "json"
#define OV_KEY_UTF8 "utf8"

#define OV_KEY_WEBSOCKET "websocket"
#define OV_KEY_WEBSOCKETS "websockets"
#define OV_KEY_HTTP "http"
#define OV_KEY_HTTPS "https"

#define OV_KEY_RTP "rtp"
#define OV_KEY_RTCP "rtcp"
#define OV_KEY_RTCP_MUX "rtcp-mux"
#define OV_KEY_SRTP "srtp"
#define OV_KEY_DTLS_SRTP "dtls srtp"

/*
 *      ------------------------------------------------------------------------
 *
 *      MEDIA TYPE DEFINITIONS
 *
 *      ------------------------------------------------------------------------
 */

#define OV_KEY_AUDIO "audio"
#define OV_KEY_VIDEO "video"
#define OV_KEY_TEXT "text"
#define OV_KEY_DATA "data"

#define OV_KEY_MIME "mime"
#define OV_KEY_EXTENSION "extension"

/*
 *      ------------------------------------------------------------------------
 *
 *      MEDIA KEY DEFINITIONS
 *
 *      ------------------------------------------------------------------------
 */

#define OV_KEY_PROTOCOL_SAVPF "UDP/TLS/RTP/SAVPF"
#define OV_KEY_PROTOCOL_SAVP "UDP/TLS/RTP/SAVP"

#define OV_KEY_ADDRESS_TYPE "address type"
#define OV_KEY_ADDRESS "address"
#define OV_KEY_ORIGIN "origin"
#define OV_KEY_REPEAT "repeat"
#define OV_KEY_ZONE "zone"
#define OV_KEY_BANDWIDTH "bandwidth"
#define OV_KEY_PORT_RTCP "port rtcp"
#define OV_KEY_FORMAT "format"

#define OV_KEY_EMAIL "email"
#define OV_KEY_PHONE "phone"

#define OV_KEY_MEDIA "media"
#define OV_KEY_MEDIA_READY "media_ready"
#define OV_KEY_CODEC "codec"
#define OV_KEY_CODECS "codecs"
#define OV_KEY_CODEC "codec"

#define OV_KEY_OPUS "opus"
#define OV_KEY_G711 "g711"
#define OV_KEY_PCM "pcm"

#define OV_KEY_STREAM_PARAMETERS "stream_parameters"

#define OV_KEY_SAMPLE_RATE_HERTZ "sample_rate_hz"

#define OV_KEY_FRAME_BUFFER "frame_buffer"
#define OV_KEY_FRAME_LENGTH_USECS "frame_length_usecs"
#define OV_KEY_LENGTH "length"

#define OV_KEY_ENDIANNESS "endianness"
#define OV_KEY_LITTLE_ENDIAN "litte_endian"
#define OV_KEY_BIG_ENDIAN "big_endian"

#define OV_KEY_MODE "mode"
#define OV_KEY_LAW "law"
#define OV_KEY_SET_SYNC_HEADER "set_sync_header"

#define OV_KEY_REPOSITORY_ROOT_PATH "repository_root_path"

// MEDIA STREAM PARAMETER KEYS
#define OV_KEY_SEND "send"
#define OV_KEY_RECV "recv"
#define OV_KEY_NONE "none"

#define OV_KEY_INACTIVE "inactive"
#define OV_KEY_SEND_ONLY "sendonly"
#define OV_KEY_RECV_ONLY "recvonly"
#define OV_KEY_SEND_RECV "sendrecv"
#define OV_KEY_MAXPTIME "maxptime"
#define OV_KEY_PTIME "ptime"
#define OV_KEY_MINPTIME "minptime"

#define OV_KEY_MSID "msid"
#define OV_KEY_SSRC "ssrc"
#define OV_KEY_CSRC "csrc"

#define OV_KEY_OVERWRITE_MSID "overwrite_msid"

#define OV_KEY_NOTIFY "notify"

#define OV_KEY_MSTRACK "mstrack"
#define OV_KEY_MID "mid"

#define OV_KEY_RTPMAP "rtpmap"
#define OV_KEY_FMTP "fmtp"

#define OV_KEY_VOLUME "volume"
#define OV_KEY_VOLUME_DB "volume_db"

#define OV_KEY_PAYLOAD_TYPE "payload_type"

#define OV_KEY_TIMESTAMP_EPOCH "timestamp_epoch"

#define OV_KEY_MSID_TO_FILTER OV_KEY_MSID "_to_filter"

#define OV_KEY_VAD "vad"
#define OV_KEY_ZERO_CROSSINGS_RATE_HERTZ "zero_crossings_rate_hertz"
#define OV_KEY_POWERLEVEL_DENSITY_DB "powerlevel_density_dbfs"

#define OV_KEY_MAX_NUM_FRAMES "max_num_frames"

#define OV_KEY_NORMALIZE_INPUT "normalize_input"

#define OV_KEY_CONFIG "config"

#define OV_KEY_LIEGE "signalling_server"

#define OV_KEY_MONITORING_FILE "monitoring_file"

#define OV_KEY_RECONNECT_INTERVAL_SECS "reconnect_interval_secs"

#define OV_KEY_OFFSET_LIMIT_USECS "offset_limit_usecs"
#define OV_KEY_LOCK_TIMEOUT_USECS "lock_timeout_usecs"
#define OV_KEY_EXPIRE_INTERVAL_USECS "expire_interval (usecs)"
#define OV_KEY_EXPIRE_NONCE_USECS "expire_nonce (usecs)"
#define OV_KEY_EXPIRE_PERMISSION_USECS "expire_permission (usecs)"

#define OV_KEY_LOCK_TIMEOUT_MSECS "lock_timeout_msecs"

#define OV_KEY_MESSAGE_QUEUE_CAPACITY "message_queue_capacity"

#define OV_KEY_NUM_THREADS "num_threads"
#define OV_KEY_NUM_PORTS "num_ports"

#define OV_KEY_CACHE_ENABLED "enabled"
#define OV_KEY_CACHE_SIZES "sizes"

#define OV_KEY_MIX_MESSAGES "mix_messages"
#define OV_KEY_RTP_MESSAGES "rtp_messages"
#define OV_KEY_RTP_FRAMES "rtp_frames"
#define OV_KEY_RTP_KEEPALIVE "rtp_keepalive"
#define OV_KEY_LISTS "lists"
#define OV_KEY_VALUES "values"
#define OV_KEY_BUFFERS "buffers"
#define OV_KEY_BUFFER "buffer"

#define OV_KEY_COPY "copy"

#define OV_KEY_TIEBREAKER "tiebreaker"

#define OV_KEY_DESTINATION "destination"
#define OV_KEY_DESTINATIONS "destinations"
#define OV_KEY_SOURCE "source"
#define OV_KEY_SOURCES "sources"

#define OV_KEY_NUMBER "number"

#define OV_KEY_FRAMES "frames"
#define OV_KEY_FRAME "frame"

/*
 *      ------------------------------------------------------------------------
 *
 *      STATE DEFINITIONS
 *
 *      ------------------------------------------------------------------------
 */

#define OV_KEY_PING "ping"

#define OV_KEY_ON "on"
#define OV_KEY_OFF "off"

#define OV_KEY_DONE "done"
#define OV_KEY_INIT "init"
#define OV_KEY_RUNNING "running"

#define OV_KEY_GATHERED "gathered"
#define OV_KEY_GATHERING "gathering"

#define OV_KEY_START "start"
#define OV_KEY_STARTING "starting"
#define OV_KEY_STARTED "started"

#define OV_KEY_STOP "stop"
#define OV_KEY_STOPPING "stopping"
#define OV_KEY_STOPPED "stopped"

#define OV_KEY_CONTROLLING "controlling"
#define OV_KEY_CONTROLLED "controlled"

#define OV_KEY_ACTIVE "active"
#define OV_KEY_PASSIVE "passive"
#define OV_KEY_UNSET "unset"

#define OV_KEY_ACTIVATE "activate"
#define OV_KEY_DEACTIVATE "deactivate"

#define OV_KEY_CONCLUDE "conclude"
#define OV_KEY_CONCLUDING "concluding"
#define OV_KEY_CONCLUDED "concluded"

#define OV_KEY_COMPLETE "complete"
#define OV_KEY_COMPLETING "completing"
#define OV_KEY_COMPLETED "completed"

#define OV_KEY_VALID "valid"
#define OV_KEY_INVALID "invalid"

#define OV_KEY_VALID_FROM "valid_from"
#define OV_KEY_VALID_UNTIL "valid_until"

#define OV_KEY_BUFFERING "buffering"
#define OV_KEY_STRICT "strict"
#define OV_KEY_RESTRICT "restrict"

#define OV_KEY_DROP "drop"

#define OV_KEY_ENABLED "enabled"
#define OV_KEY_DISABLED "disabled"

#define OV_KEY_ENABLE "enable"
#define OV_KEY_DISABLE "disable"

#define OV_KEY_READY "ready"
#define OV_KEY_SELECTED "selected"
#define OV_KEY_PROFILE "profile"

/*
 *      ------------------------------------------------------------------------
 *
 *      VOCS KEY DEFINITIONS
 *
 *      ------------------------------------------------------------------------
 */

#define OV_KEY_TALK "talk"
#define OV_KEY_MONITOR "monitor"
#define OV_KEY_LISTEN "listen"
#define OV_KEY_ABBREVIATION "abbreviation"
#define OV_KEY_DOMAIN "domain"
#define OV_KEY_DOMAINS "domains"
#define OV_KEY_LAYOUT "layout"
#define OV_KEY_LAYOUT_POSITION "layout_position"
#define OV_KEY_SETTINGS "settings"
#define OV_KEY_PARTICIPANTS "participants"

#define OV_KEY_PERMISSION "permission"
#define OV_KEY_PERMISSIONS "permissions"
#define OV_KEY_PERCENT "percent"

#define OV_KEY_PTT "ptt"

#define OV_KEY_PASSWORD "password"
#define OV_KEY_PASS OV_KEY_PASSWORD

#define OV_KEY_USER "user"
#define OV_KEY_USERS "users"

#define OV_KEY_ROLE "role"
#define OV_KEY_ROLES "roles"

#define OV_KEY_LOOP "loop"
#define OV_KEY_LOOPS "loops"

#define OV_KEY_CALLEE "callee"
#define OV_KEY_CALLER "caller"
#define OV_KEY_PEER "peer"
#define OV_KEY_LOCAL "local"

#define OV_KEY_CALL "call"
#define OV_KEY_CALLS "calls"

#define OV_KEY_HANGUP "hangup"

#define OV_KEY_CHANNEL "channel"
#define OV_KEY_PROJECT "project"
#define OV_KEY_PROJECTS "projects"

#define OV_KEY_DETAILS "details"

#define OV_KEY_IN_USE "in_use"
#define OV_KEY_UNUSED "unused"
#define OV_KEY_AVAILABLE "available"

#define OV_KEY_REQUESTS "requests"
#define OV_KEY_COMMENT "comment"

#define OV_KEY_SHUTDOWN "shutdown"

/*
 *      ------------------------------------------------------------------------
 *
 *      STATE DEFINITIONS
 *
 *      ------------------------------------------------------------------------
 */

#define OV_STATE_KEY_NONE "none"
#define OV_STATE_KEY_INIT "init"

#define OV_STATE_KEY_IDLE "idle"
#define OV_STATE_KEY_RUNNING "running"

#define OV_STATE_KEY_STARTING "starting"
#define OV_STATE_KEY_STARTED "started"
#define OV_STATE_KEY_RESTARTING "restarting"

#define OV_STATE_KEY_CONCLUDING "concluding"
#define OV_STATE_KEY_COMPLETED "completed"

#define OV_STATE_KEY_CLOSING "closing"
#define OV_STATE_KEY_CLOSED "closed"

#define OV_STATE_KEY_GATHERING "gathering"
#define OV_STATE_KEY_GATHERED "gathered"

#define OV_STATE_KEY_PROGRESS "progress"
#define OV_STATE_KEY_SUCCESS "success"
#define OV_STATE_KEY_FAILED "failed"

#define OV_STATE_KEY_FROZEN "frozen"
#define OV_STATE_KEY_WAITING "waiting"

/*
 *      ------------------------------------------------------------------------
 *
 *      (ICE) KEY DEFINITIONS
 *
 *      ------------------------------------------------------------------------
 */

#define OV_KEY_FROZEN "frozen"
#define OV_KEY_WAITING "waiting"
#define OV_KEY_NEGOTIATE "negotiate"
#define OV_KEY_CONNECTIVITY "connectivity"
#define OV_KEY_NOMINATED "nominated"
#define OV_KEY_SELECTED "selected"
#define OV_KEY_HANDSHAKED "handshaked"
#define OV_KEY_USE "use"

#define OV_KEY_MAX_STREAM_CANDIDATES "max candidates per stream"

#define OV_KEY_TIEBREAKER "tiebreaker"
#define OV_KEY_TIMEOUTS "timeouts"

#define OV_KEY_ADDR "addr"
#define OV_KEY_RADDR "raddr"
#define OV_KEY_RPORT "rport"

#define OV_KEY_UFRAG "ufrag"
#define OV_KEY_UPASS "upass"

#define OV_KEY_HANDSHAKE_USEC "handshake (usec)"
#define OV_KEY_TRANSACTION_LIFETIME "transaction lifetime (usec)"
#define OV_KEY_CLEANUP_USEC "cleanup (usec)"
#define OV_KEY_CONNECTIVITY_PACE "connectivity pace (usec)"
#define OV_KEY_KEEPALIVE_PACE "keepalive pace (usec)"

/*
 *      ------------------------------------------------------------------------
 *
 *      HTTP KEY DEFINITIONS
 *
 *      ------------------------------------------------------------------------
 */

#define OV_KEY_WEBSERVER "webserver"

#define OV_KEY_HTTP "http"
#define OV_KEY_HTTPS "https"
#define OV_KEY_HTTP_MESSAGE "http message"

#define OV_KEY_IP4_ONLY "ip4_only"
#define OV_KEY_IP4 "ip4"
#define OV_KEY_IP6 "ip6"

#define OV_KEY_SIZE "size"
#define OV_KEY_MAX_CACHE "max cache"

#define OV_KEY_HEADER "header"

#define OV_KEY_METHOD "method"
#define OV_KEY_LINE "line"
#define OV_KEY_LINES "lines"

#define OV_KEY_TRANSFER "transfer"
#define OV_KEY_CHUNK "chunk"

#define OV_KEY_MAX "max"
#define OV_KEY_MIN "min"

#define OV_KEY_CERTIFICATE "certificate"
#define OV_KEY_AUTHORITY "authority"

#define OV_KEY_FILE "file"
#define OV_KEY_KEY "key"
#define OV_KEY_FUNCTIONS "functions"

#define OV_KEY_CUSTOM "custom"
#define OV_KEY_MODULES OV_KEY_CUSTOM

/*
 *      ------------------------------------------------------------------------
 *
 *      STATISTICS
 *
 *      ------------------------------------------------------------------------
 */

#define OV_KEY_ABSOLUTE "absolute"
#define OV_KEY_TOTAL "total"
#define OV_KEY_FAILED "failed"
#define OV_KEY_SUCCEEDED "succeeded"

/*----------------------------------------------------------------------------*/

#define OV_KEY_LOGGING "log"
#define OV_KEY_LEVEL "level"

#define OV_KEY_SYSTEMD "systemd"

#define OV_KEY_FORMAT "format"

#define OV_KEY_PLAIN_TEXT "text"
#define OV_KEY_JSON "json"

#define OV_KEY_ROTATE_AFTER_MESSAGES "messages_per_file"
#define OV_KEY_KEEP_FILES "num_files"

/*----------------------------------------------------------------------------*/

#define OV_KEY_NORMALIZE_INPUT "normalize_input"

#define OV_KEY_COMFORT_NOISE "noise"
#define OV_KEY_ROOT_MIX "normalize_mixed_by_root"

#define OV_KEY_REQUEST_STEP_TIMEOUT_MSECS "request_step_msecs"
#define OV_KEY_RESPONSE_TIMEOUT_MSECS "response_msecs"

#define OV_KEY_MAX_FRAMES_TO_BUFFER_PER_STREAMS "frames_to_buffer_per_stream"

#define OV_KEY_DEVICE "device"

#define OV_KEY_AVERAGE_PER_SEC "average_per_sec"
#define OV_KEY_COUNT "count"

#endif /* ov_config_keys_h */

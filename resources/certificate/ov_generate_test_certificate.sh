#!/usr/bin/env bash
#
#       Copyright (c) 2018 German Aerospace Center DLR e.V. (GSOC)
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
#       File            ov_generate_test_certificate.sh
#       Authors         Markus Toepfer
#       Date            2020-03-26
#
#       Tool to generate some test certificate and key using openssl.
#
#       ------------------------------------------------------------------------

if [ "$EUID" -ne 0 ]
  then echo "This script MUST be run as root. You may use sudo"
  exit
fi

NAME=openvocs.test
CONF=openssl.cnf
DAYS=365
RSA=4096

#       ------------------------------------------------------------------------
#       CREATE SELF SIGNED CERTIFICATE
#       ------------------------------------------------------------------------

openssl req -x509 -newkey $RSA -nodes -keyout $NAME.key -days $DAYS -out $NAME.crt -extensions req_ext -config $CONF
openssl req -x509 -newkey $RSA -nodes -keyout one.key -days $DAYS -out one.crt -extensions req_ext -config one.cnf
openssl req -x509 -newkey $RSA -nodes -keyout two.key -days $DAYS -out two.crt -extensions req_ext -config two.cnf

#       ------------------------------------------------------------------------
#       CHANGE MODESET FOR TESTS
#       ------------------------------------------------------------------------

chmod a+r $NAME.crt
chmod a+r $NAME.key

echo ""
echo "!!! NOTE !!!"
echo ""
echo "The generated certificate key file is readable by anyone."
echo "(1) This is desired for test runs."
echo "(2) This is NOGO FOR ALL OPERATIONAL SCENARIOS."
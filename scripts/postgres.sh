#!/usr/bin/env bash
#------------------------------------------------------------------------------
#
# Copyright (c) 2024 German Aerospace Center DLR e.V. (GSOC)
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#         http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# This file is part of the openvocs project. https://openvocs.org
#
#------------------------------------------------------------------------------
#
# Comprehensive description of this python script
#
# @author Michael J. Beer, DLR/GSOC
# @copyright (c) 2024 German Aerospace Center DLR e.V. (GSOC)
# @date 2024-03-18
#
# @license Apache 2.0
# @version 1.0.0
#
# @status Development
#
#------------------------------------------------------------------------------

PASSWORD=secret
USER=openvocs
DATABASE=openvocs

function deploy() {

    docker pull postgres:16

}

#-------------------------------------------------------------------------------

function run_db() { docker container stop ov_postgres
    docker container rm ov_postgres
    docker run --name ov_postgres -e POSTGRES_PASSWORD=$PASSWORD -e POSTGRES_USER=$USER -d -p 5432:5432 postgres:16

}

#-------------------------------------------------------------------------------

function client() {

    docker exec -it ov_postgres psql -U $USER $DATABASE

}

#-------------------------------------------------------------------------------

function usage() {

    echo "Control Postrges Database (dockerized)"
    echo
    echo "    $1   deploy           Setup docker container"
    echo
    echo "    $1   server           (Re-)Start Postgres server"
    echo
    echo "    $1   client           Start psql shell"

    exit 1

}

#-------------------------------------------------------------------------------

if [ X"$2" != X ]; then
    DATABASE=$2
fi

case "$1" in

    "deploy")

        deploy
        ;;

    "server")

        run_db
        ;;

    client)

        client
        ;;

    *)

        usage
        ;;

esac

exit 0

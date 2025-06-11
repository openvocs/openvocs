#!/usr/bin/env bash
#
#       Copyright 2020 German Aerospace Center DLR e.V. (GSOC)
#
#       Licensed under the Apache License, Version 2.0 (the "License");
#
#       Authors Michael J. Beer
#
###############################################################################

PACKAGE_LISTS_DIR=${OPENVOCS_ROOT}/resources/
PLUGINS_PACKAGE_LISTS_DIR=${OPENVOCS_ROOT}/src/plugins/resources/

if [ "X$OPENVOCS_ROOT" == "X" ]; then
    echo "OPENVOCS_ROOT not set - have you sourced in env.sh?"
    exit 1
fi

function usage () {

    cat <<EOM

NAME

    show_packages.sh  - Show packages that are necessary to run openvocs

SYNOPSIS

    bash show_packages.sh [FLAVOUR] [DISTRIBUTION]

DESCRIPTION

    Shows a list of packages that need to be installed in order to run openvocs
    processes.

    BEWARE: Remember to source in env.sh !!!

EOM

}

function list_supported_distributions () {

    for mapping_file in $PACKAGE_LISTS_DIR/*/*-packages.yml; do

        echo $mapping_file | sed -e 's|.*/\([^/]*\)/\([^/]*\)-packages.yml$|\1 / \2|'

    done

    echo
    echo
}

function show_packages_from() {

    MAPPINGS=$1
    if [ ! -f $MAPPINGS ]; then
        echo "$DISTRIBUTION/$FLAVOUR not found"
        exit 1
    fi

    cat $MAPPINGS | sed -ne 's/ *- *\(.*\)/\1/p'

}


if [ "X$1" == "X" ]; then

    usage

    echo
    echo "Supported DISTRIBUTION values:"
    echo

    list_supported_distributions

    exit 0

fi

FLAVOUR=$1
DISTRIBUTION=$1

if [ "X$2" != "X" ]; then
    DISTRIBUTION=$2
fi

REL_MAPPING_FILE=$FLAVOUR/$DISTRIBUTION-packages.yml
MAPPING_FILE=$PACKAGE_LISTS_DIR/$REL_MAPPING_FILE
PLUGINS_MAPPING_FILE=$PLUGINS_PACKAGE_LISTS_DIR/$REL_MAPPING_FILE


show_packages_from $MAPPING_FILE

if [ -f $PLUGINS_MAPPING_FILE ]; then
    show_packages_from $PLUGINS_MAPPING_FILE
fi

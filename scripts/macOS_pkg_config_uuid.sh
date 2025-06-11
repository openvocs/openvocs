#!/usr/bin/env bash

SDK=`xcrun --sdk macosx --show-sdk-path`

if [ -z $SDK ]; then

	echo "You MUST install xcode SDK"
	exit 1

fi

filename="/usr/local/lib/pkgconfig/uuid.pc"
mkdir -p /usr/local/lib/pkgconfig
prefix=${SDK}
includedir=${prefix}/usr/include

if [ "$EUID" -ne 0 ]; then

    echo "You MUST run this script with sudo"

    exit 1

fi

echo "creating pkgconfig file for uuid header to SDK path:"
echo "$prefix"

echo "prefix=${SDK}" > $filename
echo "includedir=${prefix}/usr/include" >> $filename
echo "Name: uuid" >> $filename
echo "Version: 1.0.0" >> $filename
echo "Description: Universally unique id library" >> $filename
echo "Requires:" >> $filename
echo "Cflags: -I${includedir}/uuid" >> $filename

echo "wrote file:"
echo "$filename"

echo "checking cflags using pkgconfig:"
echo `pkg-config --cflags uuid`

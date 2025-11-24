#!/usr/bin/env bash

function usage() {

    echo
    echo "Usage:"
    echo "$0 version minor patch buildnumber"
    echo

    exit 1

}

if [ "X$1" == "X" ]; then
    usage
fi
if [ "X$2" == "X" ]; then
    usage
fi
if [ "X$3" == "X" ]; then
    usage
fi
if [ "X$4" == "X" ]; then
    usage
fi

#make localdist

RPM_BUILD_DIR=$OPENVOCS_ROOT/build/rpmbuild_ic
cd $OPENVOCS_ROOT/build
rm  $RPM_BUILD_DIR -rf

BASE_NAME=openvocs-interconnect-$1.$2.$3
RPM_BASE_NAME=$BASE_NAME.x86_64
LIC=$RPM_BUILD_DIR/$BASE_NAME

mkdir -p $LIC/etc/openvocs/
mkdir -p $LIC/etc/systemd/system
mkdir -p  $RPM_BUILD_DIR/{BUILD,RPMS,SRPMS,SPECS,SOURCES}
cp -r localdist/etc/openvocs/ov_mc_interconnect $LIC/etc/openvocs
cp -r localdist/etc/systemd/system/ov_mc_interconnect.service $LIC/etc/systemd/system

mkdir -p $LIC/usr/bin
cp -r localdist/usr/bin/ov_mc_interconnect $LIC/usr/bin

mkdir -p $LIC/usr/lib
cp -r localdist/usr/lib/libov_mc_interconnect2.* $LIC/usr/lib

cd $RPM_BUILD_DIR

tar cf - $BASE_NAME | gzip > SOURCES/$RPM_BASE_NAME.tar.gz

cat  << EOM > SPECS/openvocs-interconnect.spec
Name:      openvocs-interconnect
Version:   $1.$2.$3
Release:   $4
Summary:   The Openvocs interconnection module
License:   Apache
URL:       https://openvocs.net
Source0:   $RPM_BASE_NAME.tar.gz
Requires:  openvocs

%description

%prep
%setup -q

%build
%install
rm -rf \$RPM_BUILD_ROOT
mkdir -p \$RPM_BUILD_ROOT
cp -r . \$RPM_BUILD_ROOT

%post
/sbin/ldconfig
%/usr/bin/systemctl daemon-reload

%clean
rm -rf \$RPM_BUILD_ROOT

%files
%dir /etc/openvocs
%config /etc/openvocs/ov_mc_interconnect
/usr/bin/ov_mc_interconnect
/etc/systemd/system/ov_mc_interconnect.service
/usr/lib/libov_*

%changelog

EOM

rpmbuild --define "_topdir $RPM_BUILD_DIR" -bb $RPM_BUILD_DIR/SPECS/openvocs-interconnect.spec

mv $RPM_BUILD_DIR/RPMS/x86_64/*.rpm  $OPENVOCS_ROOT/build

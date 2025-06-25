#
# spec file for package openvocs
#
# Copyright (c) 2023 SUSE LLC
#
# All modifications and additions to the file contributed by third parties
# remain the property of their copyright owners, unless otherwise agreed
# upon. The license for this file, and modifications and additions to the
# file, is the same license as for the pristine package itself (unless the
# license for the pristine package is not an Open Source License, in which
# case the license is the MIT License). An "Open Source License" is a
# license that conforms to the Open Source Definition (Version 1.9)
# published by the Open Source Initiative.

# Please submit bugfixes or comments via https://bugs.opensuse.org/
#


Name:           openvocs
Version:        @OV_VERSION@
Release:        @OV_BUILD_ID@
Summary:        The Openvocs Audio Conferencing Core.
License:        Apache
URL:            https://openvocs.net
Source0:        openvocs-@OV_VERSION@.tar.gz
Requires:       libopus0, libsrtp2-1, libasound2, libpulse0

%description


%prep
%setup -q

%build
%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT
cp -r . $RPM_BUILD_ROOT

%post
/sbin/ldconfig
/usr/bin/systemctl daemon-reload


%clean
rm -rf $RPM_BUILD_ROOT

%files
%dir /etc/openvocs
/etc/openvocs/ov_config.sh
%config /etc/openvocs/certificate
%config /etc/openvocs/ov_mc_alsa
%config /etc/openvocs/ov_mc_ice_proxy
%config /etc/openvocs/ov_mc_mixer
%config /etc/openvocs/ov_mc_vocs
%config /etc/openvocs/ov_mc_vad
%config /srv/openvocs/HTML/config_vocs.js
%config /srv/openvocs/HTML/config_vocs_admin.js

%dir /srv/openvocs
/srv/openvocs/HTML/*

/usr/bin/ov_alsa_cli
/usr/bin/ov_alsa_gateway
/usr/bin/ov_domain_config_verify
/usr/bin/ov_ldap_test
/usr/bin/ov_ldap_test_auth
/usr/bin/ov_ldap_user_import
/usr/bin/ov_mc_ice_proxy
/usr/bin/ov_mc_mixer
/usr/bin/ov_mc_vocs
/usr/bin/ov_password
/usr/bin/ov_rtp_cli
/usr/bin/ov_test_mc
/usr/bin/ov_mc_cli
/usr/bin/ov_stun_client
/usr/bin/ov_stun_server
/usr/bin/ov_mc_vad
/usr/bin/ov_mc_socket_debug

/usr/lib/libov_*.so*

/etc/systemd/system/ov_mc_alsa.service
/etc/systemd/system/ov_mc_ice_proxy.service
/etc/systemd/system/ov_mc_mixer@.service
/etc/systemd/system/ov_mc_mixer.target
/etc/systemd/system/ov_mc.target
/etc/systemd/system/ov_mc_vocs.service
/etc/systemd/system/ov_mc_vad.service

%changelog

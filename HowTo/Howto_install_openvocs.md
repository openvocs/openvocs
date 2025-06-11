# HowTo install openvocs

To install openvocs you must have a build ready. For information about building openvocs have a look at HowTo_build_openvocs

## install the base system

To install the base system you should build a package for your distribution. Supported distributions are Debian, Suse and Ubuntu. 

### build a package for Debian or Ubuntu:

```
make deb
```

### build a package for Suse

```
make rpm
```

Once the build is completed the package will be avaialable under the ./build folder.
e.g. for Debian based packages a file like openvocs_1.2.2-1505_amd64.deb will be build. 
You may install the package using dpkg

```
dpkg -i build/openvocs_<version>-<build_id>_amd64.deb
```

Openvocs is now installed within your system. Important folder are configuration, which is installed to /etc/openvocs and HTML sources, which are installed under /srv/openvocs/HTML.

## finish the installation 

Now move to /etc/openvocs/ov_mc_vocs. You must now use the configuration script to configure openvocs for your IP environment. 

```
sudo ./ov_config.sh <your ip address>
```

This will preconfigure all of the configuration files:

- /etc/openvocs/ov_mc_ice_proxy/config.json
- /etc/openvocs/ov_mc_vocs/config.json
- /etc/openvocs/ov_mc_mixer/config.json
- /etc/openvocs/ov_mc_vad/config.json

In addition is will generate a certificate, which may be used for openvocs operations. 
NOTE feel encouraged to use your own certificates for openvocs. 

To finish the installation you should restart the systemctl daemon

```
sudo systemctl daemon-reload
```

## start the system

To start the openvocs system use:

```
sudo systemctl start ov_mc.target
```

This is the final target for openvocs within your systemd configuration. ov_mc.target will start all subtargets and subservices. 
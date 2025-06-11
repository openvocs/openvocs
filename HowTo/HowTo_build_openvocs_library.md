# How to build openvocs library

The openvocs library may be installed to the local system for development against openvocs. 

## installing required libraries

To build openvocs some libraries must be present. 
To show required libraries you may use:

```
scripts/show_packages.sh debian
```

To install all required libraries use:

```
for item in `./scripts/show_packages.sh ubuntu`; do sudo apt install -y $item; done
```

Libraries are preconfigured for Ubuntu, Debian and Suse variants. For other distros you may install the required libraries manually. 

## buildung openvocs 

The first step on building openvocs is to source the environment variables. This should be done using:

```
source env.sh
```

Once the environment is ready openvocs may be build using:

```
make && make target_install_lib
```

This will build the whole project and install the library packages at:

- /usr/include/openvocs/
- /usr/lib/openvocs/
- /usr/lib/pkgconfig/


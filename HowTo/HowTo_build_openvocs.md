# How to build openvocs 

This short tutorial is based for linux.

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
make
```

This will build the whole project. 

## cleaning up a build

To cleanup a build use:

```
make clean
```


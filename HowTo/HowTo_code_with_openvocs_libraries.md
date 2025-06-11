# How to code with openvocs libraries. 

## Step 1 install the library

```
make target_install_lib
```

## Step 2 link to the library

Once the library is installed you may use pkgconfig to include the openvocs specific libraries within your makefile like this:

```
CLFAGS += `pkg-config --cflags libov_base2`
LIBS   += `pkg-config --libs libov_base2`
```


# openvocs services folder

## services

* **ov_mixer**  ...
* **ov_resource_manager**   ...
* **ov_webgateway** ...
* **ov_webserver** ...

## run

```bash
.\build\bin\<service name>
```

### run options

Openvocs services support 2 generic command line switches.

* `-v` ... to print the version of the binary,
* `-c` ... to a a custom configuration path to load

Some services MAY support more options. If more options are used
-h may be used to get more information about the binary.

### auto generate webserver configuration for a specific IP

1. Create a custom config for the IP 

run the script *./resources/config/webserver/ov_generate_config.sh* with the IP

e.g.
`markus@mb16:~/openvocs/code% ./resources/config/webserver/ov_generate_config.sh 192.168.1.54`

2. Start a webserver (e.g. ov_webserver_auth_example) using the generated config

e.g.
`markus@mb16:~/openvocs/code% ./build/bin/ov_webserver_auth_example -c resources/config/webserver/config.json`

3. Use Firefox on your Laptop to access the local webserver via IP 

e.g. 
https://192.168.1.54:8001

The new certificate needs to be accepted!

4. Use Firefox on a remote maschine to access the webserver via IP over the network 

e.g. 
https://192.168.1.54:8001

The new certificate needs to be accepted!

# HowTo openvocs services

The openvocs service tree will look as follows:

ov_mc.target 
|__ ov_mc_vocs.service
|__ ov_mc_vad.service
|__ ov_mc_ice_proxy.service
|__ ov_mc_mixer@.service
|__ ov_mc_mixer.target

it may contain other sub services e.g. recorder, sip gateway or alsa gateway, dependent on the installed system base. 

## ov_mc.target

Main target for openvocs. Should be used to start, stop, restart the whole system. 

### ov_mc_vocs.service

Main service of openvocs. ov_mc_vocs contains the whole business logic of openvocs including permission and access management for connections, as well as the core webserver for client delivery and client interaction. 

### ov_mc_vad.service

This service is the voice activity indication service and used exclusively for this purpose. If voice activity indication is not working as expected you may reset this service. 

### ov_mc_ice_proxy.service

This is one instance of the ice proxy, the WebRTC Endpoint within openvocs. For simplicity only one ICE service is startet by default, but you may start several ICE services in parallel e.g. for load balancing.

The ICE service is responsible for audio distribution within the system. It will route incomming audio from WebRTC connection to mutlicast media streams and outgoing streams of a users media mixer to the WebRTC connection of that user. 

### ov_mc_mixer@.service

One instance of a ov_mixer microservice. The more mixers the system starts, the more users are supported. A mixer may be started on different hardware or cloud based systems. 

### ov_mc_mixer.target

The mixer definition of the current machine. This target defines how many mixer instances shall be started. 
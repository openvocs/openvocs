#!/bin/bash
systemctl --user $1 pipewire.socket
systemctl --user $1 pipewire.service
systemctl --user $1 pulseaudio.service
systemctl --user $1 pulseaudio.socket
systemctl --user $1 pipewire-pulse.socket
systemctl --user $1 pipewire-pulse.service

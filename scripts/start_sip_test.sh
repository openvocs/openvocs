#!/usr/bin/env bash

python3 rich/generate_config.py --target_path /tmp --stype sip_gateway
python3 rich/sip.py

python3 rich/ss.py --stype liege --port 10002 --host 127.0.0.1

python3 rich/ss.py --stype sip_gateway --port 20001 --host 127.0.0.1 --client


#!/bin/bash
valgrind --xtree-memory-file=massif --xtree-memory=full --tool=massif --depth=5 --threshold=0.1 build/ov_mixer -c rich/ov_mixer_config.json

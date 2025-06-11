#!/bin/bash

PROCESS_NAME=$1

if [ "X$1" == "X" ]; then
    PROCESS_NAME="ov_mc_vocs"
fi

# Log-Datei
LOG_FILE="/tmp/memmon_$PROCESS_NAME.log"

TIMESTAMP=$(date "+%Y-%m-%d %H:%M:%S")

REPORT="$TIMESTAMP "

PIDS=$(pgrep "$PROCESS_NAME")

if [ "X$PIDS" != "X" ]; then

    for PID in $(pgrep "$PROCESS_NAME"); do

        if [ -n "$PID" ]; then
          MEMORY_USAGE=$(grep VmRSS /proc/$PID/status | awk '{print $2}')
          REPORT="$REPORT $PID ${MEMORY_USAGE}"
        fi

    done

fi

echo $REPORT
echo $REPORT >> $LOG_FILE


#!/bin/bash

process=ov_mc_vocs

if [ "X$1" != "X" ]; then
    process=$1
fi

scp -p michael@openvocs.net:/tmp/memmon_$process.log .
cat memmon_$process.log | sed -e 's/^\([^ ]*\) \([^ ]*\).* \(.*\)kB$/\1 \2 \3/' > memmon.log
gnuplot -e "set t jpeg giant size 2000,2000; set o 'memmon.jpg'; plot 'memmon.log' u 0:3 w l"
mv memmon.log $process.log
mv memmon.jpg $process.jpg


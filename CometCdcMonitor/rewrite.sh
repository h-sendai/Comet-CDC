#!/bin/sh

for i in *.h *.cpp; do
    sed -i.bak \
    -e 's/samplemonitor/cometcdcmonitor/g' \
    -e 's/SampleMonitor/CometCdcMonitor/g' \
    -e 's/SAMPLEMONITOR/COMETCDCMONITOR/g' \
    $i
done

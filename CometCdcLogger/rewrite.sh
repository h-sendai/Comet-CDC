#!/bin/sh

for i in *.h *.cpp; do
    sed -i.bak \
    -e 's/samplelogger/cometcdclogger/g' \
    -e 's/SampleLogger/CometCdcLogger/g' \
    -e 's/SAMPLELOGGER/COMETCDCLOGGER/g' \
    $i
done

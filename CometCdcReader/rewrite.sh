#!/bin/sh

for i in *.h *.cpp; do
    sed -i.bak \
    -e 's/samplereader/cometcdcreader/g' \
    -e 's/SampleReader/CometCdcReader/g' \
    -e 's/SAMPLEREADER/COMETCDCREADER/g' \
    $i
done

#!/bin/bash

set -e

rm -rf build.paho
mkdir build.paho
cd build.paho
echo "travis build dir $TRAVIS_BUILD_DIR pwd $PWD"
cmake ..
make
ctest -VV --timeout 600
cmake .. -DSENSORNET=xbee
make MQTT-SNGateway
cmake .. -DSENSORNET=udp6
make MQTT-SNGateway
cmake .. -DSENSORNET=loralink
make MQTT-SNGateway
cd ../MQTTSNGateway/GatewayTester
make


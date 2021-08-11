#!/bin/bash

build () {
    echo "Start building MQTT-SN Gateway  $1"
    
    cd $SCRIPT_DIR/..
    BDIR='build.gateway'
    if [ ! -d ./$BDIR ]; then
        mkdir $BDIR
    fi
    cd $BDIR
    cmake .. -DSENSORNET=$1 -DDEFS="${2} ${3}"
    make MQTTSNPacket
    make MQTT-SNGateway
    make MQTT-SNLogmonitor
    cd  ../MQTTSNGateway
    cp *.conf ./bin/
}

SCRIPT_DIR=$(cd $(dirname $0); pwd)

if [ $1 == "udp" ] ; then
    build $1 $2 $3
elif [ $1 == "udp6" ] ; then 
    build $1 $2 $3
elif [ $1 == "xbee" ] ; then
    build $1 $2 $3
elif [ $1 == "loralink" ]; then
    build $1 $2 $3
elif [ $1 == "rfcomm" ] ; then 
    build $1 $2 $3
elif [ $1 == "dtls" ] ; then
    build $1 $2 $3
elif [ $1 == "dtls6" ] ; then
    build dtls "${2} ${3} -DDTLS6"
elif [ $1 == "clean" ] ; then
    rm -rf ../builg.gateway
else
    echo "Usage: build.sh  [ udp | udp6 | xbee | loralink | rfcomm | dtls | dtls6 | clean]"
fi



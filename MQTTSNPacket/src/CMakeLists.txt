#*******************************************************************************
#  Copyright (c) 2017 IBM Corp.
#
#  All rights reserved. This program and the accompanying materials
#  are made available under the terms of the Eclipse Public License v1.0
#  and Eclipse Distribution License v1.0 which accompany this distribution.
#
#  The Eclipse Public License is available at
#     http://www.eclipse.org/legal/epl-v10.html
#  and the Eclipse Distribution License is available at
#    http://www.eclipse.org/org/documents/edl-v10.php.
#
#  Contributors:
#     Ian Craggs - initial version
#*******************************************************************************/


PROJECT(mqtt-sn-packet C)

ADD_LIBRARY(MQTTSNPacketClient SHARED MQTTSNConnectClient.c MQTTSNPacket.c MQTTSNSearchClient.c MQTTSNSubscribeClient.c
            MQTTSNUnsubscribeClient.c MQTTSNSerializePublish.c MQTTSNDeserializePublish.c)

ADD_LIBRARY(MQTTSNPacketServer SHARED MQTTSNConnectServer.c MQTTSNPacket.c MQTTSNSearchServer.c MQTTSNSubscribeServer.c
            MQTTSNUnsubscribeServer.c MQTTSNSerializePublish.c MQTTSNDeserializePublish.c)

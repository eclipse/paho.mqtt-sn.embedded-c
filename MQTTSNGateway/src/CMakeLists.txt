#*******************************************************************************
#  Copyright (c) 2022 a1lu
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
#     a1lu - initial version
#*******************************************************************************/
PROJECT(mqtt-sn-gateway CXX)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Build)
set(CMAKE_CXX_STANDARD 11)

SET(OS linux)

if(NOT DEFINED SENSORNET)
    set(SENSORNET udp)
endif()
message(STATUS "SENSORNET: " ${SENSORNET})

ADD_LIBRARY(mqtt-sngateway_common
       MQTTGWConnectionHandler.cpp
       MQTTGWPacket.cpp
       MQTTGWPublishHandler.cpp
       MQTTGWSubscribeHandler.cpp
       MQTTSNGateway.cpp
       MQTTSNGWBrokerRecvTask.cpp
       MQTTSNGWBrokerSendTask.cpp
       MQTTSNGWClient.cpp
       MQTTSNGWClientRecvTask.cpp
       MQTTSNGWClientSendTask.cpp
       MQTTSNGWConnectionHandler.cpp
       MQTTSNGWLogmonitor.cpp
       MQTTSNGWPacket.cpp
       MQTTSNGWPacketHandleTask.cpp
       MQTTSNGWProcess.cpp
       MQTTSNGWPublishHandler.cpp
       MQTTSNGWSubscribeHandler.cpp
       MQTTSNGWEncapsulatedPacket.cpp
       MQTTSNGWForwarder.cpp
       MQTTSNGWQoSm1Proxy.cpp
       MQTTSNGWAdapter.cpp
       MQTTSNGWAggregater.cpp
       MQTTSNGWClientList.cpp
       MQTTSNGWTopic.cpp
       MQTTSNGWAdapterManager.cpp
       MQTTSNAggregateConnectionHandler.cpp
       MQTTSNGWMessageIdTable.cpp
       MQTTSNGWAggregateTopicTable.cpp
       ${OS}/${SENSORNET}/SensorNetwork.cpp
       ${OS}/${SENSORNET}/SensorNetwork.h
       ${OS}/Timer.cpp
       ${OS}/Timer.h
       ${OS}/Network.cpp
       ${OS}/Network.h
       ${OS}/Threading.cpp
       ${OS}/Threading.h
       )

link_directories("/usr/local/lib")
link_directories("/usr/local/opt/openssl/lib")


TARGET_INCLUDE_DIRECTORIES(mqtt-sngateway_common
       PUBLIC
       .
       ${OS}
       ${OS}/${SENSORNET}
       ../../MQTTSNPacket/src
       )

TARGET_LINK_LIBRARIES(mqtt-sngateway_common
       PRIVATE
       MQTTSNPacket
       pthread
       ssl
       crypto)

ADD_EXECUTABLE(MQTT-SNGateway
       mainGateway.cpp
       )

TARGET_LINK_LIBRARIES(MQTT-SNGateway
       mqtt-sngateway_common
       )

ADD_EXECUTABLE(MQTT-SNLogmonitor
       mainLogmonitor.cpp
       )

TARGET_LINK_LIBRARIES(MQTT-SNLogmonitor
       mqtt-sngateway_common
       )

ADD_EXECUTABLE(testPFW
       tests/mainTestProcess.cpp
       tests/TestProcess.cpp
       tests/TestQue.cpp
       tests/TestTree23.cpp
       tests/TestTopics.cpp
       tests/TestTopicIdMap.cpp
       tests/TestTask.cpp
       )
TARGET_LINK_LIBRARIES(testPFW
       mqtt-sngateway_common
       )


ADD_TEST(NAME testPFW
       WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/..
       COMMAND testPFW -f ./gateway.conf)

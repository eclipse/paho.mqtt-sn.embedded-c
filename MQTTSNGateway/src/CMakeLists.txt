SET(OS linux)
SET(SENSORNET udp)

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

TARGET_INCLUDE_DIRECTORIES(mqtt-sngateway_common
       PUBLIC
       .
       ${OS}
       ${OS}/${SENSORNET}
       )

TARGET_LINK_LIBRARIES(mqtt-sngateway_common
       PRIVATE
       MQTTSNPacketClient
       MQTTSNPacketServer
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


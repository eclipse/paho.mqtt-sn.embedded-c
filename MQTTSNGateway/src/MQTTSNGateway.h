/**************************************************************************************
 * Copyright (c) 2016, Tomoaki Yamaguchi
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Tomoaki Yamaguchi - initial API and implementation and/or initial documentation
 **************************************************************************************/
#ifndef MQTTSNGATEWAY_H_
#define MQTTSNGATEWAY_H_

#include "MQTTSNGWProcess.h"
#include "MQTTSNGWClient.h"
#include "MQTTSNPacket.h"
#include "MQTTGWPacket.h"

namespace MQTTSNGW
{
/*==========================================================
 *           Gateway default parameters
 ===========================================================*/
#define DEFAULT_KEEP_ALIVE_TIME     (900)  // 900 secs = 15 mins
#define DEFAULT_MAX_CLIENTS         (100)  // Number of Clients can be handled.
#define DEFAULT_MQTT_VERSION          (4)  // Defualt MQTT version
#define DEFAULT_INFLIGHTMESSAGE      (10)  // Number of inflight messages

/*=================================
 *    Starting prompt
 ==================================*/
#define GATEWAY_VERSION " * Version: 0.3.3"

#define PAHO_COPYRIGHT0 " * MQTT-SN Transparent Gateway"
#define PAHO_COPYRIGHT1 " * Part of Project Paho in Eclipse"
#define PAHO_COPYRIGHT2 " * (http://git.eclipse.org/c/paho/org.eclipse.paho.mqtt-sn.embedded-c.git/)"
#define PAHO_COPYRIGHT3 " * Author : Tomoaki YAMAGUCHI"
#define PAHO_COPYRIGHT4 " ***************************************************************************"
/*==========================================================
 *           Log Formats
 *
 *           RED    : \x1b[0m\x1b[1;31m
 *           green  : \x1b[0m\x1b[0;32m
 *           yellow : \x1b[0m\x1b[0;33m
 *           blue   : \x1b[0m\x1b[0;34m
 *           white  : \x1b[0m\x1b[0;37m
 ===========================================================*/
#define CLIENT      "Client"
#define CLIENTS     "Clients"
#define LEFTARROW   "<---"
#define RIGHTARROW  "--->"

#define FORMAT_Y_G_G_NL        "\n%s   \x1b[0m\x1b[0;33m%-18s\x1b[0m\x1b[0;32m%-6s%-34.32s \x1b[0m\x1b[0;34m%s\x1b[0m\x1b[0;37m\n"
#define FORMAT_Y_G_G             "%s   \x1b[0m\x1b[0;33m%-18s\x1b[0m\x1b[0;32m%-6s%-34.32s \x1b[0m\x1b[0;34m%s\x1b[0m\x1b[0;37m\n"
#define FORMAT_Y_Y_G             "%s   \x1b[0m\x1b[0;33m%-18s%-6s\x1b[0m\x1b[0;32m%-34.32s \x1b[0m\x1b[0;34m%s\x1b[0m\x1b[0;37m\n"
#define FORMAT_Y_W_G             "%s   \x1b[0m\x1b[0;33m%-18s\x1b[0m\x1b[0;37m%-6s\x1b[0m\x1b[0;32m%-34.32s \x1b[0m\x1b[0;34m%s\x1b[0m\x1b[0;37m\n"
#define FORMAT_Y_Y_W             "%s   \x1b[0m\x1b[0;33m%-18s%-6s\x1b[0m\x1b[0;37m%-34.32s \x1b[0m\x1b[0;34m%s\x1b[0m\x1b[0;37m\n"

#define FORMAT_G_MSGID_G_G_NL  "\n%s   \x1b[0m\x1b[0;32m%-11s%-5s  %-6s%-34.32s \x1b[0m\x1b[0;34m%s\x1b[0m\x1b[0;37m\n"
#define FORMAT_G_MSGID_G_G       "%s   \x1b[0m\x1b[0;32m%-11s%-5s  %-6s%-34.32s \x1b[0m\x1b[0;34m%s\x1b[0m\x1b[0;37m\n"
#define FORMAT_G_MSGID_W_G       "%s   \x1b[0m\x1b[0;32m%-11s%-5s  \x1b[0m\x1b[0;37m%-6s\x1b[0m\x1b[0;32m%-34.32 s\x1b[0m\x1b[0;34m%s\x1b[0m\x1b[0;37m\n"
#define FORMAT_G_MSGID_Y_W       "%s   \x1b[0m\x1b[0;32m%-11s%-5s  \x1b[0m\x1b[0;33m%-6s\x1b[0m\x1b[0;37m%-34.32s \x1b[0m\x1b[0;34m%s\x1b[0m\x1b[0;37m\n"

#define FORMAT_W_MSGID_Y_W_NL  "\n%s   %-11s%-5s  \x1b[0m\x1b[0;33m%-6s\x1b[0m\x1b[0;37m%-34.32s \x1b[0m\x1b[0;34m%s\x1b[0m\x1b[0;37m\n"
#define FORMAT_W_MSGID_Y_W       "%s   %-11s%-5s  \x1b[0m\x1b[0;33m%-6s\x1b[0m\x1b[0;37m%-34.32s \x1b[0m\x1b[0;34m%s\x1b[0m\x1b[0;37m\n"
#define FORMAT_W_MSGID_W_G       "%s   %-11s%-5s  %-6s\x1b[0m\x1b[0;32m%-34.32s \x1b[0m\x1b[0;34m%s\x1b[0m\x1b[0;37m\n"
#define FORMAT_W_MSGID_G_G       "%s   %-11s%-5s  \x1b[0m\x1b[0;32m%-6s%-34.32s \x1b[0m\x1b[0;34m%s\x1b[0m\x1b[0;37m\n"

#define FORMAT_BL_NL           "\n%s   \x1b[0m\x1b[0;34m%-18s%-6s%-34.32s %s\x1b[0m\x1b[0;37m\n"
#define FORMAT_W_NL            "\n%s   %-18s%-6s%-34.32s %s\n"

#define ERRMSG_HEADER            "\x1b[0m\x1b[0;31mError:"
#define ERRMSG_FOOTER            "\x1b[0m\x1b[0;37m"

/*=====================================
      Predefined TopicId for OTA
  ====================================*/
//#define OTA_CLIENTS
#define PREDEFINEDID_OTA_REQ       (0x0001)
#define PREDEFINEDID_OTA_READY     (0x0002)
#define PREDEFINEDID_OTA_NO_CLIENT (0x0003)

/*=====================================
         Class Event
  ====================================*/
enum EventType{
	Et_NA = 0,
	EtTimeout,
	EtBrokerRecv,
	EtBrokerSend,
	EtClientRecv,
	EtClientSend,
	EtBroadcast,
	EtSocketAlive
};


class Event{
public:
	Event();
	Event(EventType);
	~Event();
	EventType getEventType(void);
	void setClientRecvEvent(Client*, MQTTSNPacket*);
	void setClientSendEvent(Client*, MQTTSNPacket*);
	void setBrokerRecvEvent(Client*, MQTTGWPacket*);
	void setBrokerSendEvent(Client*, MQTTGWPacket*);
	void setBrodcastEvent(MQTTSNPacket*);  // ADVERTISE and GWINFO
	void setTimeout(void);                 // Required by EventQue<Event>.timedwait()
	Client* getClient(void);
	MQTTSNPacket* getMQTTSNPacket(void);
	MQTTGWPacket* getMQTTGWPacket(void);

private:
	EventType   _eventType;
	Client*     _client;
	MQTTSNPacket* _mqttSNPacket;
	MQTTGWPacket* _mqttGWPacket;
};

/*=====================================
 Class EventQue
 ====================================*/
class EventQue
{
public:
	EventQue();
	~EventQue();
	Event* wait(void);
	Event* timedwait(uint16_t millsec);
	int post(Event*);
	int size();

private:
	Que<Event> _que;
	Mutex      _mutex;
	Semaphore  _sem;
};

/*
 *  GatewayParams
 */
typedef struct
{
	uint8_t* loginId;
	uint8_t* password;
	uint16_t keepAlive;
	uint8_t  gatewayId;
	uint8_t  mqttVersion;
	uint16_t maxInflightMsgs;
	uint8_t* gatewayName;
}GatewayParams;

/*=====================================
     Class Gateway
 =====================================*/
class Gateway: public MultiTaskProcess{
public:
	Gateway();
	~Gateway();
	virtual void initialize(int argc, char** argv);
	void run(void);

	EventQue* getPacketEventQue(void);
	EventQue* getClientSendQue(void);
	EventQue* getBrokerSendQue(void);
	ClientList* getClientList(void);
	SensorNetwork* getSensorNetwork(void);
	LightIndicator* getLightIndicator(void);
	GatewayParams* getGWParams(void);

private:
	ClientList _clientList;
	EventQue   _packetEventQue;
	EventQue   _brokerSendQue;
	EventQue   _clientSendQue;
	LightIndicator _lightIndicator;
	GatewayParams  _params;
	SensorNetwork  _sensorNetwork;
};

}

#endif /* MQTTSNGATEWAY_H_ */

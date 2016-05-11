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
 *           Log Formats
 ===========================================================*/
#define BROKER      "Broker"
#define GATEWAY     "Gateway"
#define CLIENT      "Client"
#define CLIENTS     "Clients"
#define LEFTARROW   "<---"
#define RIGHTARROW  "--->"


#define FORMAT_WHITE_NL          "%s   %-18s%-6s%-26s%s\n"
#define FORMAT_WH_NL           "\n%s   %-18s%-6s%-26s%s\n"
#define FORMAT_WH_MSGID          "%s   %-11s%-5s  %-6s%-26s%s\n"
#define FORMAT_WH_MSGID_NL     "\n%s   %-11s%-5s  %-6s%-26s%s\n"
#define FORMAT_WH_GR             "%s   %-18s%-6s\x1b[0m\x1b[32m%-26s\x1b[0m\x1b[37m%s\n"
#define FORMAT_WH_GR_MSGID       "%s   %-11s%-5s  %-6s\x1b[0m\x1b[32m%-26s\x1b[0m\x1b[37m%s\n"
#define FORMAT_WH_GR_MSGID_NL  "\n%s   %-11s%-5s  %-6s\x1b[0m\x1b[32m%-26s\x1b[0m\x1b[37m%s\n"

#define FORMAT_GR                "%s   \x1b[0m\x1b[32m%-18s%-6s\x1b[0m\x1b[37m%-26s%s\n"
#define FORMAT_GR_NL           "\n%s   \x1b[0m\x1b[32m%-18s%-6s\x1b[0m\x1b[37m%-26s%s\n"
#define FORMAT_GR_MSGID          "%s   \x1b[0m\x1b[32m%-11s%-5s  %-6s%-26s\x1b[0m\x1b[37m%s\n"
#define FORMAT_GR_MSGID_NL     "\n%s   \x1b[0m\x1b[32m%-11s%-5s  %-6s%-26s\x1b[0m\x1b[37m%s\n"
#define FORMAT_GR_WH_MSGID       "%s   \x1b[0m\x1b[32m%-11s%-5s  %-6s\x1b[0m\x1b[37m%-26s%s\n"
#define FORMAT_GR_WH_MSGID_NL  "\n%s   \x1b[0m\x1b[32m%-11s%-5s  %-6s\x1b[0m\x1b[37m%-26s%s\n"

#define FORMAT_YE                "%s   \x1b[0m\x1b[33m%-18s%-6s%-44s\x1b[0m\x1b[37m%s\n"
#define FORMAT_YE_NL           "\n%s   \x1b[0m\x1b[33m%-18s%-6s%-26s\x1b[0m\x1b[37m%s\n"
#define FORMAT_YE_WH             "%s   \x1b[0m\x1b[33m%-18s%-6s\x1b[0m\x1b[37m%-26s\x1b[0m\x1b[37m%s\n"
#define FORMAT_YE_WH_NL        "\n%s   \x1b[0m\x1b[33m%-18s%-6s\x1b[0m\x1b[37m%-26s\x1b[0m\x1b[37m%s\n"
#define FORMAT_YE_GR             "%s   \x1b[0m\x1b[33m%-18s%-6s\x1b[0m\x1b[32m%-26s\x1b[0m\x1b[37m%s\n"
#define FORMAT_YE_GR_MSGID       "%s   \x1b[0m\x1b[33m%-11s%-5s  %-6s\x1b[0m\x1b[32m%-26s\x1b[0m\x1b[37m%s\n"

#define FORMAT_CY_ANY            "%s   \x1b[0m\x1b[36m%-18s%-6s%-44s\x1b[0m\x1b[37m%s\n"
#define FORMAT_CY                "%s   \x1b[0m\x1b[36m%-18s%-6s%-26s\x1b[0m\x1b[37m%s\n"
#define FORMAT_CY_NL           "\n%s   \x1b[0m\x1b[36m%-18s%-6s%-26s\x1b[0m\x1b[37m%s\n"

#define FORMAT_BL_NL           "\n%s   \x1b[0m\x1b[34m%-18s%-6s%-26s\x1b[0m\x1b[37m%s\n"
#define FORMAT_RED               "%s   \x1b[0m\x1b[31m%-18s%-6s%-44s\x1b[0m\x1b[37m%s\n"
#define FORMAT_RED_NL          "\n%s   \x1b[0m\x1b[31m%-18s%-6s%-26s\x1b[0m\x1b[37m%s\n"
#define ERRMSG_HEADER            "\x1b[0m\x1b[31mError:"

/*==========================================================
 *           Gateway default parameters
 ===========================================================*/
#define DEFAULT_KEEP_ALIVE_TIME     (900)  // 900 secs = 15 mins
#define DEFAULT_MAX_CLIENTS         (100)  // Number of Clients can be handled.
#define DEFAULT_MQTT_VERSION          (4)  // Defualt MQTT version
#define DEFAULT_INFLIGHTMESSAGE      (10)  // Number of inflight messages

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

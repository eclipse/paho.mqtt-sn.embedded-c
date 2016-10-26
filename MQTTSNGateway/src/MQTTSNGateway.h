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

#include "MQTTSNGWClient.h"
#include "MQTTSNGWProcess.h"
#include "MQTTSNPacket.h"

namespace MQTTSNGW
{
/*=================================
 *    Starting prompt
 ==================================*/
#define GATEWAY_VERSION " * Version: 0.9.4"

#define PAHO_COPYRIGHT0 " * MQTT-SN Transparent Gateway"
#define PAHO_COPYRIGHT1 " * Part of Project Paho in Eclipse"
#define PAHO_COPYRIGHT2 " * (http://git.eclipse.org/c/paho/org.eclipse.paho.mqtt-sn.embedded-c.git/)"
#define PAHO_COPYRIGHT3 " * Author : Tomoaki YAMAGUCHI"
#define PAHO_COPYRIGHT4 " ***************************************************************************"
/*==========================================================
 *           Log Formats
 *
 *           RED    : \033[0m\033[1;31m
 *           green  : \033[0m\033[0;32m
 *           yellow : \033[0m\033[0;33m
 *           blue   : \033[0m\033[0;34m
 *           white  : \033[0m\033[0;37m
 ===========================================================*/
#define CLIENT      "Client"
#define CLIENTS     "Clients"
#define UNKNOWNCL   "Unknown Client !"
#define LEFTARROW   "<---"
#define RIGHTARROW  "--->"

#define FORMAT_Y_G_G_NL        "\n%s   \033[0m\033[0;33m%-18s\033[0m\033[0;32m%-6s%-34.32s \033[0m\033[0;34m%s\033[0m\033[0;37m\n"
#define FORMAT_Y_G_G             "%s   \033[0m\033[0;33m%-18s\033[0m\033[0;32m%-6s%-34.32s \033[0m\033[0;34m%s\033[0m\033[0;37m\n"
#define FORMAT_Y_Y_G             "%s   \033[0m\033[0;33m%-18s%-6s\033[0m\033[0;32m%-34.32s \033[0m\033[0;34m%s\033[0m\033[0;37m\n"
#define FORMAT_Y_W_G             "%s   \033[0m\033[0;33m%-18s\033[0m\033[0;37m%-6s\033[0m\033[0;32m%-34.32s \033[0m\033[0;34m%s\033[0m\033[0;37m\n"
#define FORMAT_Y_Y_W             "%s   \033[0m\033[0;33m%-18s%-6s\033[0m\033[0;37m%-34.32s \033[0m\033[0;34m%s\033[0m\033[0;37m\n"

#define FORMAT_G_MSGID_G_G_NL  "\n%s   \033[0m\033[0;32m%-11s%-5s  %-6s%-34.32s \033[0m\033[0;34m%s\033[0m\033[0;37m\n"
#define FORMAT_G_MSGID_G_G       "%s   \033[0m\033[0;32m%-11s%-5s  %-6s%-34.32s \033[0m\033[0;34m%s\033[0m\033[0;37m\n"
#define FORMAT_G_MSGID_W_G       "%s   \033[0m\033[0;32m%-11s%-5s  \033[0m\033[0;37m%-6s\033[0m\033[0;32m%-34.32 s\033[0m\033[0;34m%s\033[0m\033[0;37m\n"
#define FORMAT_G_MSGID_Y_W       "%s   \033[0m\033[0;32m%-11s%-5s  \033[0m\033[0;33m%-6s\033[0m\033[0;37m%-34.32s \033[0m\033[0;34m%s\033[0m\033[0;37m\n"

#define FORMAT_W_MSGID_Y_W_NL  "\n%s   %-11s%-5s  \033[0m\033[0;33m%-6s\033[0m\033[0;37m%-34.32s \033[0m\033[0;34m%s\033[0m\033[0;37m\n"
#define FORMAT_W_MSGID_Y_W       "%s   %-11s%-5s  \033[0m\033[0;33m%-6s\033[0m\033[0;37m%-34.32s \033[0m\033[0;34m%s\033[0m\033[0;37m\n"
#define FORMAT_W_MSGID_W_G       "%s   %-11s%-5s  %-6s\033[0m\033[0;32m%-34.32s \033[0m\033[0;34m%s\033[0m\033[0;37m\n"
#define FORMAT_W_MSGID_G_G       "%s   %-11s%-5s  \033[0m\033[0;32m%-6s%-34.32s \033[0m\033[0;34m%s\033[0m\033[0;37m\n"

#define FORMAT_BL_NL           "\n%s   \033[0m\033[0;34m%-18s%-6s%-34.32s %s\033[0m\033[0;37m\n"
#define FORMAT_W_NL            "\n%s   %-18s%-6s%-34.32s %s\n"

#define ERRMSG_HEADER            "\033[0m\033[0;31mError:"
#define ERRMSG_FOOTER            "\033[0m\033[0;37m"

/*=====================================
      Predefined TopicId for OTA
  ====================================*/
#define OTA_CLIENTS
#define PREDEFINEDID_OTA_REQ       (0x0ff0)
#define PREDEFINEDID_OTA_READY     (0x0ff1)
#define PREDEFINEDID_OTA_NO_CLIENT (0x0ff2)

/*=====================================
         Class Event
  ====================================*/
enum EventType{
	Et_NA = 0,
	EtStop,
	EtTimeout,
	EtBrokerRecv,
	EtBrokerSend,
	EtClientRecv,
	EtClientSend,
	EtBroadcast,
	EtSensornetSend
};


class Event{
public:
	Event();
	~Event();
	EventType getEventType(void);
	void setClientRecvEvent(Client*, MQTTSNPacket*);
	void setClientSendEvent(Client*, MQTTSNPacket*);
	void setBrokerRecvEvent(Client*, MQTTGWPacket*);
	void setBrokerSendEvent(Client*, MQTTGWPacket*);
	void setBrodcastEvent(MQTTSNPacket*);  // ADVERTISE and GWINFO
	void setTimeout(void);                 // Required by EventQue<Event>.timedwait()
	void setStop(void);
	void setClientSendEvent(SensorNetAddress*, MQTTSNPacket*);
	Client* getClient(void);
	SensorNetAddress* getSensorNetAddress(void);
	MQTTSNPacket* getMQTTSNPacket(void);
	MQTTGWPacket* getMQTTGWPacket(void);

private:
	EventType   _eventType;
	Client*     _client;
	SensorNetAddress* _sensorNetAddr;
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
	void setMaxSize(uint16_t maxSize);
	void post(Event*);
	int  size();

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
	char* configName;
	char* clientListName;
	char* loginId;
	char* password;
	uint16_t keepAlive;
	uint8_t  gatewayId;
	uint8_t  mqttVersion;
	uint16_t maxInflightMsgs;
	char* gatewayName;
	char* brokerName;
	char* port;
	char* portSecure;
	char* rootCApath;
	char* rootCAfile;
	char* certKey;
	char* privateKey;
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

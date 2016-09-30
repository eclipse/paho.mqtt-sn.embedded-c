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

#include "MQTTSNGateway.h"
#include "SensorNetwork.h"
#include "MQTTSNGWProcess.h"
#include <string.h>
using namespace MQTTSNGW;

char* currentDateTime(void);

/*=====================================
 Class Gateway
 =====================================*/
Gateway::Gateway()
{
	theMultiTaskProcess = this;
	theProcess = this;
	_params.loginId = 0;
	_params.password = 0;
	_packetEventQue.setMaxSize(MAX_INFLIGHTMESSAGES * MAX_CLIENTS);
}

Gateway::~Gateway()
{
	if ( _params.loginId )
	{
		free(_params.loginId);
	}
	if ( _params.password )
	{
		free(_params.password);
	}
}

void Gateway::initialize(int argc, char** argv)
{
	char param[MQTTSNGW_PARAM_MAX];
	MultiTaskProcess::initialize(argc, argv);
	resetRingBuffer();

	_params.gatewayId = 0;
	if (getParam("GatewayID", param) == 0)
	{
		_params.gatewayId = atoi(param);
	}

	if (_params.gatewayId == 0 || _params.gatewayId > 255)
	{
		throw Exception( "Gateway::initialize: invalid Gateway Id");
	}

	if (getParam("GatewayName", param) == 0)
	{
		_params.gatewayName = (uint8_t*) strdup(param);
	}

	_params.mqttVersion = DEFAULT_MQTT_VERSION;
	if (getParam("MQTTVersion", param) == 0)
	{
		_params.mqttVersion = atoi(param);
	}

	_params.maxInflightMsgs = DEFAULT_MQTT_VERSION;
	if (getParam("MaxInflightMsgs", param) == 0)
	{
		_params.maxInflightMsgs = atoi(param);
	}

	_params.keepAlive = DEFAULT_KEEP_ALIVE_TIME;

	if (getParam("KeepAlive", param) == 0)
	{
		_params.keepAlive = atoi(param);
	}

	if (_params.keepAlive > 65536)
	{
		throw Exception("Gateway::initialize: KeepAliveTime is grater than 65536 Secs");
	}

	if (getParam("LoginID", param) == 0)
	{
		_params.loginId = (uint8_t*) strdup(param);
	}

	if (getParam("Password", param) == 0)
	{
		_params.password = (uint8_t*) strdup(param);
	}

	if (getParam("ClientAuthentication", param) == 0)
	{
		string fileName;
		if (!strcasecmp(param, "YES"))
		{
			if (getParam("ClientsList", param) == 0)
			{
				fileName = string(param);
			}
			else
			{
				fileName = *getConfigDirName() + string(MQTTSNGW_CLIENT_LIST);
			}

			if (!_clientList.authorize(fileName.c_str()))
			{
				throw Exception("Gateway::initialize: No client list defined by configuration.");
			}
		}
	}
}

void Gateway::run(void)
{
	_lightIndicator.redLight(true);
	WRITELOG("\n%s", PAHO_COPYRIGHT4);
	WRITELOG("\n%s\n", PAHO_COPYRIGHT0);
	WRITELOG("%s\n", PAHO_COPYRIGHT1);
	WRITELOG("%s\n", PAHO_COPYRIGHT2);
	WRITELOG(" *\n%s\n", PAHO_COPYRIGHT3);
	WRITELOG("%s\n", GATEWAY_VERSION);
	WRITELOG("%s\n", PAHO_COPYRIGHT4);
	WRITELOG("\n%s %s has been started.\n                    listening on, %s\n", currentDateTime(), _params.gatewayName, _sensorNetwork.getDescription());

	if ( getClientList()->isAuthorized() )
	{
		WRITELOG("\nClient authentication is required by the configuration settings.\n\n");
	}

	/* execute threads & wait StopProcessEvent MQTTSNGWPacketHandleTask posts by CTL-C */

	MultiTaskProcess::run();
	WRITELOG("%s MQTT-SN Gateway stoped\n", currentDateTime());
	_lightIndicator.allLightOff();
}

EventQue* Gateway::getPacketEventQue()
{
	return &_packetEventQue;
}

EventQue* Gateway::getClientSendQue()
{
	return &_clientSendQue;
}

EventQue* Gateway::getBrokerSendQue()
{
	return &_brokerSendQue;
}

ClientList* Gateway::getClientList()
{
	return &_clientList;
}

SensorNetwork* Gateway::getSensorNetwork()
{
	return &_sensorNetwork;
}

LightIndicator* Gateway::getLightIndicator()
{
	return &_lightIndicator;
}

GatewayParams* Gateway::getGWParams(void)
{
	return &_params;
}

/*=====================================
 Class EventQue
 =====================================*/
EventQue::EventQue()
{

}

EventQue::~EventQue()
{

}

void  EventQue::setMaxSize(uint16_t maxSize)
{
	_que.setMaxSize((int)maxSize);
}

Event* EventQue::wait(void)
{
	Event* ev;
	while ( true )
	{
		_sem.wait();
		_mutex.lock();
		ev = _que.front();
		_que.pop();
		_mutex.unlock();
		if ( ev )
		{
			return ev;
		}
	}
}

Event* EventQue::timedwait(uint16_t millsec)
{
	Event* ev;
	_sem.timedwait(millsec);
	_mutex.lock();

	if (_que.size() == 0)
	{
		ev = new Event();
		ev->setTimeout();
	}
	else
	{
		ev = _que.front();
		_que.pop();
		if ( !ev )
		{
			ev = new Event();
			ev->setTimeout();
		}
	}
	_mutex.unlock();
	return ev;
}

int EventQue::post(Event* ev)
{
	int rc = 0;
	_mutex.lock();
	rc = _que.post(ev);
	_sem.post();
	_mutex.unlock();
	return rc;
}

int EventQue::size()
{
	_mutex.lock();
	int sz = _que.size();
	_mutex.unlock();
	return sz;
}


/*=====================================
 Class Event
 =====================================*/
Event::Event()
{
	_eventType = Et_NA;
	_client = 0;
	_mqttSNPacket = 0;
	_mqttGWPacket = 0;
}

Event::Event(EventType type)
{
	_eventType = type;
	_client = 0;
	_mqttSNPacket = 0;
	_mqttGWPacket = 0;
}

Event::~Event()
{
	if (_mqttSNPacket)
	{
		delete _mqttSNPacket;
	}

	if (_mqttGWPacket)
	{
		delete _mqttGWPacket;
	}
}

EventType Event::getEventType()
{
	return _eventType;
}

void Event::setClientSendEvent(Client* client, MQTTSNPacket* packet)
{
	_client = client;
	_eventType = EtClientSend;
	_mqttSNPacket = packet;
}

void Event::setBrokerSendEvent(Client* client, MQTTGWPacket* packet)
{
	_client = client;
	_eventType = EtBrokerSend;
	_mqttGWPacket = packet;
}

void Event::setClientRecvEvent(Client* client, MQTTSNPacket* packet)
{
	_client = client;
	_eventType = EtClientRecv;
	_mqttSNPacket = packet;
}

void Event::setBrokerRecvEvent(Client* client, MQTTGWPacket* packet)
{
	_client = client;
	_eventType = EtBrokerRecv;
	_mqttGWPacket = packet;
}

void Event::setTimeout(void)
{
	_eventType = EtTimeout;
}

void Event::setBrodcastEvent(MQTTSNPacket* msg)
{
	_mqttSNPacket = msg;
	_eventType = EtBroadcast;
}

Client* Event::getClient(void)
{
	return _client;
}

MQTTSNPacket* Event::getMQTTSNPacket()
{
	return _mqttSNPacket;
}

MQTTGWPacket* Event::getMQTTGWPacket(void)
{
	return _mqttGWPacket;
}

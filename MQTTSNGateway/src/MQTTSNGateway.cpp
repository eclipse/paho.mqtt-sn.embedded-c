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
#include "MQTTSNGWDefines.h"
#include "MQTTSNGateway.h"
#include "SensorNetwork.h"
#include "MQTTSNGWProcess.h"
#include "MQTTSNGWVersion.h"
#include "MQTTSNGWQoSm1Proxy.h"
#include "MQTTSNGWClient.h"
#include <string.h>
using namespace MQTTSNGW;

char* currentDateTime(void);

/*=====================================
 Class Gateway
 =====================================*/
MQTTSNGW::Gateway* theGateway = nullptr;

Gateway::Gateway(void)
{
    theMultiTaskProcess = this;
    theProcess = this;
    _packetEventQue.setMaxSize(MAX_INFLIGHTMESSAGES * MAX_CLIENTS);
    _clientList = new ClientList();
    _adapterManager = new AdapterManager(this);
    _topics = new Topics();
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
	if ( _params.gatewayName )
	{
		free(_params.gatewayName);
	}
	if ( _params.brokerName )
	{
		free(_params.brokerName);
	}
	if ( _params.port )
	{
		free(_params.port);
	}
	if ( _params.portSecure )
	{
		free(_params.portSecure);
	}
	if ( _params.certKey )
	{
		free(_params.certKey);
	}
	if ( _params.privateKey )
	{
		free(_params.privateKey);
	}
	if ( _params.rootCApath )
	{
		free(_params.rootCApath);
	}
	if ( _params.rootCAfile )
	{
		free(_params.rootCAfile);
	}
	if ( _params.clientListName )
	{
		free(_params.clientListName);
	}
	if ( _params.configName )
	{
		free(_params.configName);
	}

    if ( _params.qosMinusClientListName )
    {
        free(_params.qosMinusClientListName);
    }

    if ( _adapterManager )
    {
        delete _adapterManager;
    }
    if ( _clientList )
    {
        delete _clientList;
    }

    if ( _topics )
	{
		delete _topics;
	}
}

int Gateway::getParam(const char* parameter, char* value)
{
    return MultiTaskProcess::getParam(parameter, value);
}

void Gateway::initialize(int argc, char** argv)
{
	char param[MQTTSNGW_PARAM_MAX];
	string fileName;
    theGateway = this;

	MultiTaskProcess::initialize(argc, argv);
	resetRingBuffer();

	_params.configDir = *getConfigDirName();
    fileName = _params.configDir + *getConfigFileName();
    _params.configName = strdup(fileName.c_str());

	if (getParam("BrokerName", param) == 0)
	{
		_params.brokerName = strdup(param);
	}
	if (getParam("BrokerPortNo", param) == 0)
	{
		_params.port = strdup(param);
	}
	if (getParam("BrokerSecurePortNo", param) == 0)
	{
		_params.portSecure = strdup(param);
	}

	if (getParam("CertKey", param) == 0)
	{
		_params.certKey = strdup(param);
	}
	if (getParam("PrivateKey", param) == 0)
		{
			_params.privateKey = strdup(param);
		}
	if (getParam("RootCApath", param) == 0)
	{
		_params.rootCApath = strdup(param);
	}
	if (getParam("RootCAfile", param) == 0)
	{
		_params.rootCAfile = strdup(param);
	}

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
		_params.gatewayName = strdup(param);
	}

	if (_params.gatewayName == 0 )
	{
		throw Exception( "Gateway::initialize: Gateway Name is missing.");
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

	if (getParam("LoginID", param) == 0)
	{
		_params.loginId = strdup(param);
	}

	if (getParam("Password", param) == 0)
	{
		_params.password = strdup(param);
	}

	if (getParam("ClientAuthentication", param) == 0)
	{
		if (!strcasecmp(param, "YES"))
		{
			_params.clientAuthentication = true;
		}
	}

	/*  ClientList and Adapters  Initialize  */
	_adapterManager->initialize();

	bool aggregate = _adapterManager->isAggregaterActive();
	_clientList->initialize(aggregate);

	/*  Setup predefined topics  */
	_clientList->setPredefinedTopics(aggregate);
}

void Gateway::run(void)
{
    /* write prompts */
	_lightIndicator.redLight(true);
	WRITELOG("\n%s", PAHO_COPYRIGHT4);
	WRITELOG("\n%s\n", PAHO_COPYRIGHT0);
	WRITELOG("%s\n", PAHO_COPYRIGHT1);
	WRITELOG("%s\n", PAHO_COPYRIGHT2);
	WRITELOG(" *\n%s\n", PAHO_COPYRIGHT3);
	WRITELOG(" * Version: %s\n", PAHO_GATEWAY_VERSION);
	WRITELOG("%s\n", PAHO_COPYRIGHT4);
	WRITELOG("\n%s %s has been started.\n\n", currentDateTime(), _params.gatewayName);
	WRITELOG(" ConfigFile: %s\n", _params.configName);

	if ( _params.clientListName )
	{
		WRITELOG(" ClientList: %s\n", _params.clientListName);
	}

    if (  _params.predefinedTopicFileName )
    {
        WRITELOG(" PreDefFile: %s\n", _params.predefinedTopicFileName);
    }

	WRITELOG(" SensorN/W:  %s\n", _sensorNetwork.getDescription());
	WRITELOG(" Broker:     %s : %s, %s\n", _params.brokerName, _params.port, _params.portSecure);
	WRITELOG(" RootCApath: %s\n", _params.rootCApath);
	WRITELOG(" RootCAfile: %s\n", _params.rootCAfile);
	WRITELOG(" CertKey:    %s\n", _params.certKey);
	WRITELOG(" PrivateKey: %s\n\n\n", _params.privateKey);


	/* Run Tasks until CTRL+C entred */
	MultiTaskProcess::run();

	/* stop Tasks */
	Event* ev = new Event();
	ev->setStop();
	_packetEventQue.post(ev);
	ev = new Event();
	ev->setStop();
	_brokerSendQue.post(ev);
	ev = new Event();
	ev->setStop();
	_clientSendQue.post(ev);

	/* wait until all Task stop */
	MultiTaskProcess::waitStop();

	WRITELOG("\n%s MQTT-SN Gateway  stoped\n\n", currentDateTime());
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
	return _clientList;
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

AdapterManager* Gateway::getAdapterManager(void)
{
    return _adapterManager;
}

Topics* Gateway::getTopics(void)
{
    return _topics;
}

bool Gateway::hasSecureConnection(void)
{
	return (  _params.certKey
			&& _params.privateKey
			&& _params.rootCApath
			&& _params.rootCAfile );
}
/*=====================================
 Class EventQue
 =====================================*/
EventQue::EventQue()
{

}

EventQue::~EventQue()
{
	_mutex.lock();
	while (_que.size() > 0)
	{
		delete _que.front();
		_que.pop();
	}
	_mutex.unlock();
}

void  EventQue::setMaxSize(uint16_t maxSize)
{
	_que.setMaxSize((int)maxSize);
}

Event* EventQue::wait(void)
{
	Event* ev = nullptr;

	while(ev == nullptr)
	{
		if ( _que.size() == 0 )
		{
			_sem.wait();
		}
		_mutex.lock();
		ev = _que.front();
		_que.pop();
		_mutex.unlock();
	}
	return ev;
}

Event* EventQue::timedwait(uint16_t millsec)
{
	Event* ev;
	if ( _que.size() == 0 )
	{
		_sem.timedwait(millsec);
	}
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
	}
	_mutex.unlock();
	return ev;
}

void EventQue::post(Event* ev)
{
	if ( ev )
	{
		_mutex.lock();
		if ( _que.post(ev) )
		{
			_sem.post();
		}
		else
		{
			delete ev;
		}
		_mutex.unlock();
	}
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

}

Event::~Event()
{
	if (_sensorNetAddr)
	{
		delete _sensorNetAddr;
	}

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

void Event::setStop(void)
{
	_eventType = EtStop;
}

void Event::setBrodcastEvent(MQTTSNPacket* msg)
{
	_mqttSNPacket = msg;
	_eventType = EtBroadcast;
}

void Event::setClientSendEvent(SensorNetAddress* addr, MQTTSNPacket* msg)
{
	_eventType = EtSensornetSend;
	_sensorNetAddr = addr;
	_mqttSNPacket = msg;
}

Client* Event::getClient(void)
{
	return _client;
}

SensorNetAddress* Event::getSensorNetAddress(void)
{
	return _sensorNetAddr;
}

MQTTSNPacket* Event::getMQTTSNPacket()
{
	return _mqttSNPacket;
}

MQTTGWPacket* Event::getMQTTGWPacket(void)
{
	return _mqttGWPacket;
}



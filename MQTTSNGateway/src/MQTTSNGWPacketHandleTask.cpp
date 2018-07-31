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
 *    Tieto Poland Sp. z o.o. - Gateway improvements
 **************************************************************************************/

#include "MQTTSNGWDefines.h"
#include "MQTTSNGWPacketHandleTask.h"
#include "MQTTSNGWProcess.h"
#include "MQTTGWPacket.h"
#include "MQTTSNGWClient.h"
#include "MQTTSNGWProcess.h"
#include "MQTTGWConnectionHandler.h"
#include "MQTTGWPublishHandler.h"
#include "MQTTGWSubscribeHandler.h"
#include "MQTTSNGWConnectionHandler.h"
#include "MQTTSNGWPublishHandler.h"
#include "MQTTSNGWSubscribeHandler.h"
#include <string.h>

using namespace std;
using namespace MQTTSNGW;

#define EVENT_QUE_TIME_OUT  2000      // 2000 msecs
char* currentDateTime(void);
/*=====================================
 Class PacketHandleTask
 =====================================*/

PacketHandleTask::PacketHandleTask(Gateway* gateway)
{
	_gateway = gateway;
	_gateway->attach((Thread*)this);
	_mqttConnection = new MQTTGWConnectionHandler(_gateway);
	_mqttPublish = new MQTTGWPublishHandler(_gateway);
	_mqttSubscribe = new MQTTGWSubscribeHandler(_gateway);
	_mqttsnConnection = new MQTTSNConnectionHandler(_gateway);
	_mqttsnPublish = new MQTTSNPublishHandler(_gateway);
	_mqttsnSubscribe = new MQTTSNSubscribeHandler(_gateway);
}

/**
 *  Destructor is called by Gateway's destructor indirectly.
 */
PacketHandleTask::~PacketHandleTask()
{
	if ( _mqttConnection )
	{
		delete _mqttConnection;
	}
	if ( _mqttPublish )
	{
		delete _mqttPublish;
	}
	if ( _mqttSubscribe )
	{
		delete _mqttSubscribe;
	}
	if ( _mqttsnConnection )
	{
		delete _mqttsnConnection;
	}
	if ( _mqttsnPublish )
	{
		delete _mqttsnPublish;
	}
	if ( _mqttsnSubscribe )
	{
		delete _mqttsnSubscribe;
	}
}

void PacketHandleTask::run()
{
	Event* ev = 0;
	EventQue* eventQue = _gateway->getPacketEventQue();
	Client* client = 0;
	MQTTSNPacket* snPacket = 0;
	MQTTGWPacket* brPacket = 0;
	char msgId[6];
	memset(msgId, 0, 6);

	_advertiseTimer.start(_gateway->getGWParams()->keepAlive * 1000UL);

	while (true)
	{
		/* wait Event */
		ev = eventQue->timedwait(EVENT_QUE_TIME_OUT);

		if (ev->getEventType() == EtStop)
		{
			WRITELOG("%s PacketHandleTask stopped.\n", currentDateTime());
			delete ev;
			return;
		}

		if (ev->getEventType() == EtTimeout)
		{
			/*------ Check Keep Alive Timer & send Advertise ------*/
			if (_advertiseTimer.isTimeup())
			{
				_mqttsnConnection->sendADVERTISE();
				_advertiseTimer.start(_gateway->getGWParams()->keepAlive * 1000UL);
			}

			/*------ Check QoS-1 Proxy   Connect or PINGREQ ------*/
			QoSm1Proxy* pxy = _gateway->getQoSm1Proxy();
			if ( pxy )
			{
			    pxy->checkConnection();
			}
		}

		/*------    Handle SEARCHGW Message     ---------*/
		else if (ev->getEventType() == EtBroadcast)
		{
			snPacket = ev->getMQTTSNPacket();
			_mqttsnConnection->handleSearchgw(snPacket);
		}

		/*------    Handle Messages form Clients      ---------*/
		else if (ev->getEventType() == EtClientRecv)
		{
			client = ev->getClient();
			snPacket = ev->getMQTTSNPacket();

			DEBUGLOG("     PacketHandleTask gets %s %s from the client.\n", snPacket->getName(), snPacket->getMsgId(msgId));

			switch (snPacket->getType())
			{
			case MQTTSN_CONNECT:
				_mqttsnConnection->handleConnect(client, snPacket);
				break;
			case MQTTSN_WILLTOPIC:
				_mqttsnConnection->handleWilltopic(client, snPacket);
				break;
			case MQTTSN_WILLMSG:
				_mqttsnConnection->handleWillmsg(client, snPacket);
				break;
			case MQTTSN_DISCONNECT:
				_mqttsnConnection->handleDisconnect(client, snPacket);
				break;
			case MQTTSN_WILLMSGUPD:
				_mqttsnConnection->handleWillmsgupd(client, snPacket);
				break;
			case MQTTSN_PINGREQ:
				_mqttsnConnection->handlePingreq(client, snPacket);
				break;
			case MQTTSN_PUBLISH:
				_mqttsnPublish->handlePublish(client, snPacket);
				break;
			case MQTTSN_PUBACK:
				_mqttsnPublish->handlePuback(client, snPacket);
				break;
			case MQTTSN_PUBREC:
				_mqttsnPublish->handleAck(client, snPacket, PUBREC);
				break;
			case MQTTSN_PUBREL:
				_mqttsnPublish->handleAck(client, snPacket, PUBREL);
				break;
			case MQTTSN_PUBCOMP:
				_mqttsnPublish->handleAck(client, snPacket, PUBCOMP);
				break;
			case MQTTSN_REGISTER:
				_mqttsnPublish->handleRegister(client, snPacket);
				break;
			case MQTTSN_REGACK:
			    _mqttsnPublish->handleRegAck(client, snPacket);
				break;
			case MQTTSN_SUBSCRIBE:
				_mqttsnSubscribe->handleSubscribe(client, snPacket);
				break;
			case MQTTSN_UNSUBSCRIBE:
				_mqttsnSubscribe->handleUnsubscribe(client, snPacket);
				break;
			default:
				break;
			}

			/* Reset the Timer for PINGREQ. */
			client->updateStatus(snPacket);
		}

		/*------  Handle Messages form Broker      ---------*/
		else if (ev->getEventType() == EtBrokerRecv)
		{
			client = ev->getClient();
			brPacket = ev->getMQTTGWPacket();
			DEBUGLOG("     PacketHandleTask gets %s %s from the broker.\n", brPacket->getName(), brPacket->getMsgId(msgId));
			switch (brPacket->getType())
			{
			case CONNACK:
				_mqttConnection->handleConnack(client, brPacket);
				break;
			case PINGRESP:
				_mqttConnection->handlePingresp(client, brPacket);
				break;
			case PUBLISH:
				_mqttPublish->handlePublish(client, brPacket);
				break;
			case PUBACK:
				_mqttPublish->handlePuback(client, brPacket);
				break;
			case PUBREC:
				_mqttPublish->handleAck(client, brPacket, PUBREC);
				break;
			case PUBREL:
				_mqttPublish->handleAck(client, brPacket, PUBREL);
				break;
			case PUBCOMP:
				_mqttPublish->handleAck(client, brPacket, PUBCOMP);
				break;
			case SUBACK:
				_mqttSubscribe->handleSuback(client, brPacket);
				break;
			case UNSUBACK:
				_mqttSubscribe->handleUnsuback(client, brPacket);
				break;
			default:
				break;
			}
		}
		delete ev;
	}
}


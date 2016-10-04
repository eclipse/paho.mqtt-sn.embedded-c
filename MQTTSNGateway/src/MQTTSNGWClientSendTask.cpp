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
#include "MQTTSNGWClientSendTask.h"
#include "MQTTSNGWPacket.h"
#include "MQTTSNGWPacket.h"
#include "MQTTSNGateway.h"

using namespace MQTTSNGW;
using namespace std;
char* currentDateTime(void);
/*=====================================
 Class ClientSendTask
 =====================================*/
ClientSendTask::ClientSendTask(Gateway* gateway)
{
	_gateway = gateway;
	_gateway->attach((Thread*)this);
	_sensorNetwork = _gateway->getSensorNetwork();
}

ClientSendTask::~ClientSendTask()
{

}

void ClientSendTask::run()
{
	Client* client = 0;
	MQTTSNPacket* packet = 0;

	while (true)
	{
		Event* ev = _gateway->getClientSendQue()->wait();

		if (ev->getEventType() == EtStop)
		{
			delete ev;
			break;
		}
		if (ev->getEventType() == EtClientSend)
		{
			client = ev->getClient();
			packet = ev->getMQTTSNPacket();
			packet->unicast(_sensorNetwork, client->getSensorNetAddress());
		}
		else if (ev->getEventType() == EtBroadcast)
		{
			packet = ev->getMQTTSNPacket();
			packet->broadcast(_sensorNetwork);
		}
		else if (ev->getEventType() == EtSensornetSend)
		{
			packet = ev->getMQTTSNPacket();
			packet->unicast(_sensorNetwork, ev->getSensorNetAddress());
		}

		log(client, packet);
		delete ev;
	}
}

void ClientSendTask::log(Client* client, MQTTSNPacket* packet)
{
	char pbuf[SIZE_OF_LOG_PACKET * 3];
	char msgId[6];
	const char* clientId = client ? (const char*)client->getClientId() : NONACTCLT ;

	switch (packet->getType())
	{
	case MQTTSN_ADVERTISE:
	case MQTTSN_GWINFO:
	case MQTTSN_PINGRESP:
		WRITELOG(FORMAT_Y_W_G, currentDateTime(), packet->getName(), RIGHTARROW, CLIENTS, packet->print(pbuf));
		break;
	case MQTTSN_CONNACK:
	case MQTTSN_DISCONNECT:
	case MQTTSN_WILLTOPICREQ:
	case MQTTSN_WILLMSGREQ:
	case MQTTSN_WILLTOPICRESP:
	case MQTTSN_WILLMSGRESP:
		WRITELOG(FORMAT_Y_W_G, currentDateTime(), packet->getName(), RIGHTARROW, clientId, packet->print(pbuf));
		break;
	case MQTTSN_REGISTER:
	case MQTTSN_PUBLISH:
		WRITELOG(FORMAT_W_MSGID_W_G, currentDateTime(), packet->getName(), packet->getMsgId(msgId), RIGHTARROW, clientId,	packet->print(pbuf));
		break;
	case MQTTSN_REGACK:
	case MQTTSN_PUBACK:
	case MQTTSN_PUBREC:
	case MQTTSN_PUBREL:
	case MQTTSN_PUBCOMP:
	case MQTTSN_SUBACK:
	case MQTTSN_UNSUBACK:
		WRITELOG(FORMAT_W_MSGID_W_G, currentDateTime(), packet->getName(), packet->getMsgId(msgId), RIGHTARROW, clientId,	packet->print(pbuf));
		break;
	default:
		break;
	}
}

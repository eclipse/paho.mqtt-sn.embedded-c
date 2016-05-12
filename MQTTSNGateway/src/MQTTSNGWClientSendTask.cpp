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

		log(client, packet);
		delete ev;
	}
}

void ClientSendTask::log(Client* client, MQTTSNPacket* packet)
{
	char pbuf[SIZEOF_LOG_PACKET * 3];
	char msgId[6];

	switch (packet->getType())
	{
	case MQTTSN_ADVERTISE:
	case MQTTSN_GWINFO:
		WRITELOG(FORMAT_CY_NL, currentDateTime(), packet->getName(), RIGHTARROW, CLIENTS, packet->print(pbuf));
		break;
	case MQTTSN_CONNACK:
	case MQTTSN_DISCONNECT:
		WRITELOG(FORMAT_YE_WH, currentDateTime(), packet->getName(), RIGHTARROW, client->getClientId(), packet->print(pbuf));
		break;
	case MQTTSN_WILLTOPICREQ:
	case MQTTSN_WILLMSGREQ:
	case MQTTSN_WILLTOPICRESP:
	case MQTTSN_WILLMSGRESP:
		WRITELOG(FORMAT_GR, currentDateTime(), packet->getName(), RIGHTARROW, client->getClientId(), packet->print(pbuf));
		break;
	case MQTTSN_REGISTER:
	case MQTTSN_PUBLISH:
		WRITELOG(FORMAT_GR_WH_MSGID_NL, currentDateTime(), packet->getName(), packet->getMsgId(msgId), RIGHTARROW, client->getClientId(),	packet->print(pbuf));
		break;
	case MQTTSN_REGACK:
	case MQTTSN_PUBACK:
	case MQTTSN_PUBREC:
	case MQTTSN_PUBREL:
	case MQTTSN_PUBCOMP:
	case MQTTSN_SUBACK:
	case MQTTSN_UNSUBACK:
		WRITELOG(FORMAT_GR_WH_MSGID, currentDateTime(), packet->getName(), packet->getMsgId(msgId), RIGHTARROW, client->getClientId(),	packet->print(pbuf));
		break;
	case MQTTSN_PINGRESP:
		WRITELOG(FORMAT_CY, currentDateTime(), packet->getName(), RIGHTARROW, client->getClientId(), packet->print(pbuf));
		break;
	default:
		break;
	}
}

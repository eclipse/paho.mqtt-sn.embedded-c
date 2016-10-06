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

#include "MQTTSNGWClientRecvTask.h"
#include "MQTTSNGateway.h"
#include <string.h>
char* currentDateTime(void);
/*=====================================
 Class ClientRecvTask
 =====================================*/
ClientRecvTask::ClientRecvTask(Gateway* gateway)
{
	_gateway = gateway;
	_gateway->attach((Thread*)this);
	_sensorNetwork = _gateway->getSensorNetwork();
}

ClientRecvTask::~ClientRecvTask()
{

}

/**
 * Initialize SensorNetwork
 */
void ClientRecvTask::initialize(int argc, char** argv)
{
	if ( _sensorNetwork->initialize() < 0 )
	{
		throw Exception(" Can't open the sensor network.\n");
	}
}

/*
 * Receive a packet from clients via sensor netwwork
 * and generate a event to execute the packet handling  procedure
 * of MQTTSNPacketHandlingTask.
 */
void ClientRecvTask::run()
{
	Event* ev = 0;
	Client* client = 0;

	while (true)
	{
		MQTTSNPacket* packet = new MQTTSNPacket();
		int packetLen = packet->recv(_sensorNetwork);

		if (CHK_SIGINT)
		{
			delete packet;
			return;
		}

		if (packetLen < 2 )
		{
			delete packet;
			continue;
		}

		if ( packet->getType() <= MQTTSN_ADVERTISE || packet->getType() == MQTTSN_GWINFO )
		{
			delete packet;
			continue;
		}

		if ( packet->getType() == MQTTSN_SEARCHGW )
		{
			/* write log and post Event */
			log(0, packet);
			ev = new Event();
			ev->setBrodcastEvent(packet);
			_gateway->getPacketEventQue()->post(ev);
			continue;
		}

		/* get client from the ClientList of Gateway by sensorNetAddress. */
		client = _gateway->getClientList()->getClient(_sensorNetwork->getSenderAddress());

		if ( client )
		{
			/* write log and post Event */
			log(client, packet);
			ev = new Event();
			ev->setClientRecvEvent(client,packet);
			_gateway->getPacketEventQue()->post(ev);
		}
		else
		{
			/* new client */
			if (packet->getType() == MQTTSN_CONNECT)
			{
				MQTTSNPacket_connectData data;
				memset(&data, 0, sizeof(MQTTSNPacket_connectData));
				packet->getCONNECT(&data);

				/* create a client */
				client = _gateway->getClientList()->createClient(_sensorNetwork->getSenderAddress(), &data.clientID, false, false); //_gateway->getGWParams()->secureConnection);

				if (!client)
				{
					WRITELOG("%s Client was rejected. CONNECT message has been discarded.%s\n", ERRMSG_HEADER, ERRMSG_FOOTER);
					delete packet;
					continue;
				}

				log(client, packet);

				/* set sensorNetAddress & post Event */
				client->setClientAddress(_sensorNetwork->getSenderAddress());
				ev = new Event();
				ev->setClientRecvEvent(client, packet);
				_gateway->getPacketEventQue()->post(ev);
			}
			else
			{
				log(client, packet);
				delete packet;
				/* Send DISCONNECT */
				SensorNetAddress* addr = new SensorNetAddress();
				addr->setAddress(_sensorNetwork->getSenderAddress()->getIpAddress(), _sensorNetwork->getSenderAddress()->getPortNo());
				packet = new MQTTSNPacket();
				packet->setDISCONNECT(0);
				ev = new Event();
				ev->setClientSendEvent(addr, packet);
				_gateway->getClientSendQue()->post(ev);
				continue;
			}
		}
	}
}

void ClientRecvTask::log(Client* client, MQTTSNPacket* packet)
{
	char pbuf[SIZE_OF_LOG_PACKET * 3];
	char msgId[6];
	const char* clientId = client ? (const char*)client->getClientId() : NONACTCLT ;

	switch (packet->getType())
	{
	case MQTTSN_SEARCHGW:
	case MQTTSN_PINGREQ:
		WRITELOG(FORMAT_Y_G_G_NL, currentDateTime(), packet->getName(), LEFTARROW, CLIENT, packet->print(pbuf));
		break;
	case MQTTSN_CONNECT:
		WRITELOG(FORMAT_Y_G_G_NL, currentDateTime(), packet->getName(), LEFTARROW, clientId, packet->print(pbuf));
		break;
	case MQTTSN_DISCONNECT:
	case MQTTSN_WILLTOPICUPD:
	case MQTTSN_WILLMSGUPD:
	case MQTTSN_WILLTOPIC:
	case MQTTSN_WILLMSG:
		WRITELOG(FORMAT_Y_G_G, currentDateTime(), packet->getName(), LEFTARROW, clientId, packet->print(pbuf));
		break;
	case MQTTSN_PUBLISH:
	case MQTTSN_REGISTER:
	case MQTTSN_SUBSCRIBE:
	case MQTTSN_UNSUBSCRIBE:
		WRITELOG(FORMAT_G_MSGID_G_G_NL, currentDateTime(), packet->getName(), packet->getMsgId(msgId), LEFTARROW, clientId, packet->print(pbuf));
		break;
	case MQTTSN_REGACK:
	case MQTTSN_PUBACK:
	case MQTTSN_PUBREC:
	case MQTTSN_PUBREL:
	case MQTTSN_PUBCOMP:
		WRITELOG(FORMAT_G_MSGID_G_G, currentDateTime(), packet->getName(), packet->getMsgId(msgId), LEFTARROW, clientId, packet->print(pbuf));
		break;
	default:
		WRITELOG(FORMAT_W_NL, currentDateTime(), packet->getName(), LEFTARROW, clientId, packet->print(pbuf));
		break;
	}
}

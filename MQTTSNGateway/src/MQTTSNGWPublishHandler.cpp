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

#include "MQTTSNGWPublishHandler.h"
#include "MQTTSNGWPacket.h"
#include "MQTTGWPacket.h"
#include "MQTTSNGateway.h"
#include "MQTTSNGWClient.h"
#include <string.h>
using namespace std;
using namespace MQTTSNGW;

MQTTSNPublishHandler::MQTTSNPublishHandler(Gateway* gateway)
{
	_gateway = gateway;
}

MQTTSNPublishHandler::~MQTTSNPublishHandler()
{

}

void MQTTSNPublishHandler::handlePublish(Client* client, MQTTSNPacket* packet)
{
	uint8_t dup;
	int qos;
	uint8_t retained;
	uint16_t msgId;
	MQTTSN_topicid topicid;
	uint8_t* payload;
	int payloadlen;
	Publish pub;
	char  shortTopic[2];

	if ( !client->isActive() )
	{
		/* Reply DISCONNECT to the client */
		Event* ev = new Event();
		MQTTSNPacket* disconnect = new MQTTSNPacket();
		disconnect->setDISCONNECT(0);
		ev->setClientSendEvent(client, disconnect);
		_gateway->getClientSendQue()->post(ev);
		return;
	}

	if ( packet->getPUBLISH(&dup, &qos, &retained, &msgId, &topicid, &payload, &payloadlen) ==0 )
	{
		return;
	}
	pub.msgId = msgId;
	pub.header.bits.dup = dup;
	pub.header.bits.qos = qos;
	pub.header.bits.retain = retained;

	Topic* topic = 0;

	if ( topicid.type == MQTTSN_TOPIC_TYPE_PREDEFINED)
	{
		if(msgId)
		{
			/* Reply PubAck to the client */
			MQTTSNPacket* pubAck = new MQTTSNPacket();
			pubAck->setPUBACK( topicid.data.id, msgId, MQTTSN_RC_ACCEPTED);
			Event* ev1 = new Event();
			ev1->setClientSendEvent(client, pubAck);
			_gateway->getClientSendQue()->post(ev1);
		}

#ifdef OTA_CLIENTS
		if ( topicid.data.id == PREDEFINEDID_OTA_REQ )
		{
			uint8_t clientId[MAX_CLIENTID_LENGTH + 1];

			if ( payloadlen <= MAX_CLIENTID_LENGTH )
			{
				memcpy(clientId, payload, payloadlen);
				clientId[payloadlen] = 0;
				Client* cl = _gateway->getClientList()->getClient(clientId);

				if ( cl )
				{
					WRITELOG("\033[0m\033[0;33m OTA Client : %s\033[0m\033[0;37m\n",cl->getClientId());
					MQTTSNPacket* pubota = new MQTTSNPacket();
					pubota->setPUBLISH(0, 0, 0, 0, topicid, 0, 0);
					cl->setOTAClient(client);
					Event* evt = new Event();
					evt->setClientSendEvent(cl, pubota);
					_gateway->getClientSendQue()->post(evt);
				}
				else
				{
					MQTTSNPacket* publish = new MQTTSNPacket();
					topicid.data.id = PREDEFINEDID_OTA_NO_CLIENT;
					publish->setPUBLISH(0, 0, 0, 0, topicid, clientId, (uint16_t)strlen((const char*)clientId));
					Event* evt = new Event();
					evt->setClientSendEvent(client, publish);
					_gateway->getClientSendQue()->post(evt);
				}
			}
		}
		else if ( topicid.data.id == PREDEFINEDID_OTA_READY )
		{
			Client* cl = client->getOTAClient();
			if ( cl )
			{
				WRITELOG("\033[0m\033[0;33m OTA Manager : %s\033[0m\033[0;37m\n",cl->getClientId());
				MQTTSNPacket* pubota = new MQTTSNPacket();
				pubota->setPUBLISH(0, 0, 0, 0, topicid, payload, payloadlen);
				client->setOTAClient(0);
				Event* evt = new Event();
				evt->setClientSendEvent(cl, pubota);
				_gateway->getClientSendQue()->post(evt);
			}
		}
#endif
		return;
	}

	if( topicid.type ==  MQTTSN_TOPIC_TYPE_SHORT )
	{
		shortTopic[0] = topicid.data.short_name[0];
		shortTopic[1] = topicid.data.short_name[1];
		pub.topic = shortTopic;
		pub.topiclen = 2;
	}

	if ( topicid.type == MQTTSN_TOPIC_TYPE_NORMAL )
	{
		topic = client->getTopics()->getTopic(topicid.data.id);
		if( !topic && msgId && qos > 0 )
		{
			/* Reply PubAck with INVALID_TOPIC_ID to the client */
			MQTTSNPacket* pubAck = new MQTTSNPacket();
			pubAck->setPUBACK( topicid.data.id, msgId, MQTTSN_RC_REJECTED_INVALID_TOPIC_ID);
			Event* ev1 = new Event();
			ev1->setClientSendEvent(client, pubAck);
			_gateway->getClientSendQue()->post(ev1);
			return;
		}
		if ( topic )
		{
			pub.topic = (char*)topic->getTopicName()->data();
			pub.topiclen = topic->getTopicName()->length();
		}
	}
	/* Save a msgId & a TopicId pare for PUBACK */
	if( msgId && qos > 0 )
	{
		client->setWaitedPubTopicId(msgId, topicid.data.id, topicid.type);
	}

	pub.payload = (char*)payload;
	pub.payloadlen = payloadlen;

	MQTTGWPacket* publish = new MQTTGWPacket();
	publish->setPUBLISH(&pub);
	Event* ev1 = new Event();
	ev1->setBrokerSendEvent(client, publish);
	_gateway->getBrokerSendQue()->post(ev1);
}

void MQTTSNPublishHandler::handlePuback(Client* client, MQTTSNPacket* packet)
{
	uint16_t topicId;
	uint16_t msgId;
	uint8_t rc;

	if ( client->isActive() )
	{
		MQTTGWPacket* pubAck = new MQTTGWPacket();

		if ( packet->getPUBACK(&topicId, &msgId, &rc) == 0 )
		{
			return;
		}

		if ( rc == MQTTSN_RC_ACCEPTED)
		{
			pubAck->setAck(PUBACK, msgId);
			Event* ev1 = new Event();
			ev1->setBrokerSendEvent(client, pubAck);
			_gateway->getBrokerSendQue()->post(ev1);
		}
		else if ( rc == MQTTSN_RC_REJECTED_INVALID_TOPIC_ID)
		{
			WRITELOG("  PUBACK   %d : Invalid Topic ID\n", msgId);
		}
	}
}

void MQTTSNPublishHandler::handleAck(Client* client, MQTTSNPacket* packet, uint8_t packetType)
{
	uint16_t msgId;

	if ( client->isActive() )
	{
		if ( packet->getACK(&msgId) == 0 )
		{
			return;
		}
		MQTTGWPacket* ackPacket = new MQTTGWPacket();
		ackPacket->setAck(packetType, msgId);
		Event* ev1 = new Event();
		ev1->setBrokerSendEvent(client, ackPacket);
		_gateway->getBrokerSendQue()->post(ev1);
	}
}

void MQTTSNPublishHandler::handleRegister(Client* client, MQTTSNPacket* packet)
{
	uint16_t id;
	uint16_t msgId;
	MQTTSNString topicName;
	MQTTSN_topicid topicid;


	if ( client->isActive() || client->isAwake())
	{
		MQTTSNPacket* regAck = new MQTTSNPacket();
		if ( packet->getREGISTER(&id, &msgId, &topicName) == 0 )
		{
			return;
		}

		topicid.type = MQTTSN_TOPIC_TYPE_NORMAL;
		topicid.data.long_.len = topicName.lenstring.len;
		topicid.data.long_.name = topicName.lenstring.data;

		id = client->getTopics()->add(&topicid)->getTopicId();
		regAck->setREGACK(id, msgId, MQTTSN_RC_ACCEPTED);
		Event* ev = new Event();
		ev->setClientSendEvent(client, regAck);
		_gateway->getClientSendQue()->post(ev);
	}
}

void MQTTSNPublishHandler::handleRegAck( Client* client, MQTTSNPacket* packet)
{
    uint16_t id;
    uint16_t msgId;
    uint8_t rc;
    if ( client->isActive() || client->isAwake())
    {
        if ( packet->getREGACK(&id, &msgId, &rc) == 0 )
        {
            return;
        }

        MQTTSNPacket* regAck = client->getWaitREGACKPacketList()->getPacket(msgId);
        if ( regAck != 0 )
        {
            client->getWaitREGACKPacketList()->erase(msgId);
            Event* ev = new Event();
            ev->setClientSendEvent(client, regAck);
            _gateway->getClientSendQue()->post(ev);
        }
    }

}

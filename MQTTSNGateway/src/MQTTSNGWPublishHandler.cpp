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
		WRITELOG("     The client is not active. status = %s\n",  client->getStatus());
		Event* ev = new Event();
		MQTTSNPacket* disconnect = new MQTTSNPacket();
		disconnect->setDISCONNECT(0);
		ev->setClientSendEvent(client, disconnect);
		_gateway->getClientSendQue()->post(ev);
		return;
	}

	packet->getPUBLISH(&dup, &qos, &retained, &msgId, &topicid, &payload, &payloadlen);
	pub.msgId = msgId;
	pub.header.bits.dup = dup;
	pub.header.bits.qos = qos;
	pub.header.bits.retain = retained;

	Topic* topic = 0;

	if ( topicid.type == MQTTSN_TOPIC_TYPE_PREDEFINED)
	{
		/*
		 * 		ToDo:  PUBLISH predefined Topic procedures.
		 */

		if(msgId)
		{
			/* Reply PubAck to the client */
			MQTTSNPacket* pubAck = new MQTTSNPacket();
			pubAck->setPUBACK( topicid.data.id, msgId, MQTTSN_RC_ACCEPTED);
			Event* ev1 = new Event();
			ev1->setClientSendEvent(client, pubAck);
			_gateway->getClientSendQue()->post(ev1);
		}
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
			/* Reply PubAck of INVALID_TOPIC_ID to the client */
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

	MQTTGWPacket* pulish = new MQTTGWPacket();
	pulish->setPUBLISH(&pub);
	Event* ev1 = new Event();
	ev1->setBrokerSendEvent(client, pulish);
	_gateway->getBrokerSendQue()->post(ev1);
}

void MQTTSNPublishHandler::handlePuback(Client* client, MQTTSNPacket* packet)
{
	uint16_t topicId;
	uint16_t msgId;
	uint8_t rc;

	if ( !client->isActive() )
	{
		return;
	}
	MQTTGWPacket* pubAck = new MQTTGWPacket();
	packet->getPUBACK(&topicId, &msgId, &rc);
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

void MQTTSNPublishHandler::handleAck(Client* client, MQTTSNPacket* packet, uint8_t packetType)
{
	uint16_t msgId;

	if ( !client->isActive() )
	{
		return;
	}
	packet->getACK(&msgId);
	MQTTGWPacket* ackPacket = new MQTTGWPacket();
	ackPacket->setAck(packetType, msgId);
	Event* ev1 = new Event();
	ev1->setBrokerSendEvent(client, ackPacket);
	_gateway->getBrokerSendQue()->post(ev1);
}

void MQTTSNPublishHandler::handleRegister(Client* client, MQTTSNPacket* packet)
{
	uint16_t id;
	uint16_t msgId;
	MQTTSNString topicName;
	MQTTSN_topicid topicid;


	if ( !client->isActive() )
	{
		return;
	}
	MQTTSNPacket* regAck = new MQTTSNPacket();
	packet->getREGISTER(&id, &msgId, &topicName);

	topicid.type = MQTTSN_TOPIC_TYPE_NORMAL;
	topicid.data.long_.len = topicName.lenstring.len;
	topicid.data.long_.name = topicName.lenstring.data;

	id = client->getTopics()->add(&topicid)->getTopicId();
	regAck->setREGACK(id, msgId, MQTTSN_RC_ACCEPTED);
	Event* ev = new Event();
	ev->setClientSendEvent(client, regAck);
	_gateway->getClientSendQue()->post(ev);

}

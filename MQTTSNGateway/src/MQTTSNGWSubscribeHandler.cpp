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

#include "MQTTSNGWSubscribeHandler.h"
#include "MQTTSNGWPacket.h"
#include "MQTTGWPacket.h"
#include "MQTTSNGateway.h"
#include "MQTTSNGWClient.h"

using namespace std;
using namespace MQTTSNGW;

MQTTSNSubscribeHandler::MQTTSNSubscribeHandler(Gateway* gateway)
{
	_gateway = gateway;
}

MQTTSNSubscribeHandler::~MQTTSNSubscribeHandler()
{

}

void MQTTSNSubscribeHandler::handleSubscribe(Client* client, MQTTSNPacket* packet)
{
	uint8_t dup;
	int qos;
	uint16_t msgId;
	MQTTSN_topicid topicFilter;
	Topic* topic = 0;

	if ( packet->getSUBSCRIBE(&dup, &qos, &msgId, &topicFilter) == 0 )
	{
		return;
	}

	if (topicFilter.type <= MQTTSN_TOPIC_TYPE_SHORT)
	{
		if (topicFilter.type == MQTTSN_TOPIC_TYPE_PREDEFINED)
		{
			/*----- Predefined TopicId ------*/
			MQTTSNPacket* sSuback = new MQTTSNPacket();

			if (msgId)
			{
				int rc = MQTTSN_RC_ACCEPTED;

				switch (topicFilter.data.id)
				{
				case PREDEFINEDID_OTA_REQ:  // check topicIds are defined.
				case PREDEFINEDID_OTA_READY:
				case PREDEFINEDID_OTA_NO_CLIENT:
					break;
				default:
					rc = MQTTSN_RC_REJECTED_INVALID_TOPIC_ID;
				}
				sSuback->setSUBACK(qos, topicFilter.data.id, msgId, rc);
				Event* evsuback = new Event();
				evsuback->setClientSendEvent(client, sSuback);
				_gateway->getClientSendQue()->post(evsuback);
			}
			switch (topicFilter.data.id)
			{
			case 1:
				/*
				 *  ToDo: write here Predefined Topic 01 Procedures.
				 */
				break;
			case 2:
				/*
				 *  ToDo: write here Predefined Topic 02 Procedures. so on
				 */
				break;
			default:
				break;
			}
		}
		else
		{
			MQTTGWPacket* subscribe = new MQTTGWPacket();
			topic = client->getTopics()->getTopic(&topicFilter);
			if (topic == 0)
			{
				if (topicFilter.type == MQTTSN_TOPIC_TYPE_NORMAL)
				{
					topic = client->getTopics()->add(&topicFilter);
					subscribe->setSUBSCRIBE((char*)topic->getTopicName()->c_str(), (uint8_t)qos, (uint16_t)msgId);
				}
				else if (topicFilter.type == MQTTSN_TOPIC_TYPE_SHORT)
				{
					char topic[3];
					topic[0] = topicFilter.data.short_name[0];
					topic[1] = topicFilter.data.short_name[1];
					topic[2] = 0;
					subscribe->setSUBSCRIBE(topic, (uint8_t)qos, (uint16_t)msgId);
				}
			}
			else
			{
				subscribe->setSUBSCRIBE((char*)topic->getTopicName()->c_str(), (uint8_t)qos, (uint16_t)msgId);
			}

			if ( msgId > 0 )
			{
				client->setWaitedSubTopicId(msgId, topic->getTopicId(), topicFilter.type);
			}

			Event* ev1 = new Event();
			ev1->setBrokerSendEvent(client, subscribe);
			_gateway->getBrokerSendQue()->post(ev1);
			return;
		}
	}
	else
	{
		/*-- Invalid TopicIdType --*/
		if (msgId)
		{
			MQTTSNPacket* sSuback = new MQTTSNPacket();
			sSuback->setSUBACK(qos, topicFilter.data.id, msgId, MQTTSN_RC_REJECTED_INVALID_TOPIC_ID);
			Event* evsuback = new Event();
			evsuback->setClientSendEvent(client, sSuback);
			_gateway->getClientSendQue()->post(evsuback);
		}
	}
}

void MQTTSNSubscribeHandler::handleUnsubscribe(Client* client, MQTTSNPacket* packet)
{
	uint16_t msgId;
	MQTTSN_topicid topicFilter;

	if ( packet->getUNSUBSCRIBE(&msgId, &topicFilter) == 0 )
	{
		return;
	}

	if ( topicFilter.type == MQTTSN_TOPIC_TYPE_PREDEFINED )
	{
		/*
		 *  ToDo: procedures for Predefined Topic
		 */
		return;
	}

	Topic* topic = client->getTopics()->getTopic(&topicFilter);
	MQTTGWPacket* unsubscribe = new MQTTGWPacket();

	if (topicFilter.type == MQTTSN_TOPIC_TYPE_NORMAL)
	{
		if ( topic == 0 )
		{
			if (msgId)
			{
				MQTTSNPacket* sUnsuback = new MQTTSNPacket();
				sUnsuback->setUNSUBACK(msgId);
				Event* evsuback = new Event();
				evsuback->setClientSendEvent(client, sUnsuback);
				_gateway->getClientSendQue()->post(evsuback);
			}
			delete unsubscribe;
			return;
		}
		else
		{
			unsubscribe->setUNSUBSCRIBE(topic->getTopicName()->c_str(), msgId);
		}
	}
	else if (topicFilter.type == MQTTSN_TOPIC_TYPE_SHORT)
	{
		MQTTGWPacket* unsubscribe = new MQTTGWPacket();
		char shortTopic[3];
		shortTopic[0] = topicFilter.data.short_name[0];
		shortTopic[1] = topicFilter.data.short_name[1];
		shortTopic[2] = 0;
		unsubscribe->setUNSUBSCRIBE(shortTopic, msgId);
	}
	else
	{
		delete unsubscribe;
		return;
	}

	Event* ev1 = new Event();
	ev1->setBrokerSendEvent(client, unsubscribe);
	_gateway->getBrokerSendQue()->post(ev1);
}


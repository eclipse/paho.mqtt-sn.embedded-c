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
    uint16_t topicId = 0;
    MQTTGWPacket* subscribe;
    Event* ev1;
    Event* evsuback;

	if ( packet->getSUBSCRIBE(&dup, &qos, &msgId, &topicFilter) == 0 )
	{
		return;
	}

	if ( msgId == 0 )
	{
	    return;
	}

    if ( topicFilter.type == MQTTSN_TOPIC_TYPE_PREDEFINED )
    {
        topic = client->getTopics()->getTopicById(&topicFilter);
        if ( topic )
        {
            topicId = topic->getTopicId();
            subscribe = new MQTTGWPacket();
            subscribe->setSUBSCRIBE((char*)topic->getTopicName()->c_str(), (uint8_t)qos, (uint16_t)msgId);
        }
        else
        {
            goto RespExit;
        }
    }
    else if (topicFilter.type == MQTTSN_TOPIC_TYPE_NORMAL)
    {
        topic = client->getTopics()->getTopicByName(&topicFilter);
        if ( topic  == 0 )
        {
            topic = client->getTopics()->add(&topicFilter);
            if ( topic == 0 )
            {
                WRITELOG("%s Client(%s) can't add the Topic.%s\n", ERRMSG_HEADER, client->getClientId(), ERRMSG_FOOTER);
                return;
            }
        }
        topicId = topic->getTopicId();
        subscribe = new MQTTGWPacket();

        subscribe->setSUBSCRIBE((char*)topic->getTopicName()->c_str(), (uint8_t)qos, (uint16_t)msgId);
    }
    else  //MQTTSN_TOPIC_TYPE_SHORT
    {
        char topicstr[3];
        topicstr[0] = topicFilter.data.short_name[0];
        topicstr[1] = topicFilter.data.short_name[1];
        topicstr[2] = 0;
        topicId = 0;
        subscribe = new MQTTGWPacket();
        subscribe->setSUBSCRIBE(topicstr, (uint8_t)qos, (uint16_t)msgId);
    }

    client->setWaitedSubTopicId(msgId, topicId, topicFilter.type);

    ev1 = new Event();
    ev1->setBrokerSendEvent(client, subscribe);
    _gateway->getBrokerSendQue()->post(ev1);
    return;


RespExit:
     MQTTSNPacket* sSuback = new MQTTSNPacket();
     sSuback->setSUBACK(qos, topicFilter.data.id, msgId, MQTTSN_RC_NOT_SUPPORTED);
     evsuback = new Event();
     evsuback->setClientSendEvent(client, sSuback);
     _gateway->getClientSendQue()->post(evsuback);
}

void MQTTSNSubscribeHandler::handleUnsubscribe(Client* client, MQTTSNPacket* packet)
{
	uint16_t msgId;
	MQTTSN_topicid topicFilter;
    MQTTGWPacket* unsubscribe = 0;;

	if ( packet->getUNSUBSCRIBE(&msgId, &topicFilter) == 0 )
	{
		return;
	}

	if ( msgId == 0 )
    {
	    return;
    }

	Topic* topic = client->getTopics()->getTopicById(&topicFilter);

	if (topicFilter.type == MQTTSN_TOPIC_TYPE_SHORT)
	{
	    char shortTopic[3];
        shortTopic[0] = topicFilter.data.short_name[0];
        shortTopic[1] = topicFilter.data.short_name[1];
        shortTopic[2] = 0;
        unsubscribe = new MQTTGWPacket();
        unsubscribe->setUNSUBSCRIBE(shortTopic, msgId);
	}
	else
	{
	    if ( topic == 0 )
        {
            MQTTSNPacket* sUnsuback = new MQTTSNPacket();
            sUnsuback->setUNSUBACK(msgId);
            Event* evsuback = new Event();
            evsuback->setClientSendEvent(client, sUnsuback);
            _gateway->getClientSendQue()->post(evsuback);
            return;
        }
        else
        {
            unsubscribe = new MQTTGWPacket();
            unsubscribe->setUNSUBSCRIBE(topic->getTopicName()->c_str(), msgId);
        }
	}

	Event* ev1 = new Event();
	ev1->setBrokerSendEvent(client, unsubscribe);
	_gateway->getBrokerSendQue()->post(ev1);
}


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
#include "MQTTSNGWAggregater.h"
#include "MQTTSNGateway.h"
#include "MQTTSNGWClient.h"
#include "MQTTSNGWAdapter.h"
#include "MQTTSNGWAdapterManager.h"
#include "MQTTSNGWMessageIdTable.h"
#include "MQTTSNGWTopic.h"
#include <stdio.h>
#include <string.h>
#include <string>

using namespace MQTTSNGW;

Aggregater::Aggregater(Gateway* gw) : Adapter(gw)
{
	_gateway = gw;
}

Aggregater::~Aggregater(void)
{

}

void Aggregater::initialize(void)
{
    char param[MQTTSNGW_PARAM_MAX];

    if (_gateway->getParam("AggregatingGateway", param) == 0 )
    {
        if (!strcasecmp(param, "YES") )
        {
           /* Create Aggregated Clients */
        	_gateway->getClientList()->setClientList(AGGREGATER_TYPE);

        	string name = _gateway->getGWParams()->gatewayName;
        	setup(name.c_str(), Atype_Aggregater);
        	_isActive = true;
        }
    }

    //testMessageIdTable();

}

bool Aggregater::isActive(void)
{
	return _isActive;
}

uint16_t Aggregater::msgId(void)
{
	return Adapter::getSecureClient()->getNextPacketId();
}

Client* Aggregater::convertClient(uint16_t msgId, uint16_t* clientMsgId)
{
	return _msgIdTable.getClientMsgId(msgId, clientMsgId);
}


uint16_t Aggregater::addMessageIdTable(Client* client, uint16_t msgId)
{
	/* set Non secure client`s nextMsgId. otherwise Id is duplicated.*/

	MessageIdElement* elm = _msgIdTable.add(this, client, msgId);
	if ( elm == nullptr )
	{
		return 0;
	}
	else
	{
		return elm->_msgId;
	}
}

uint16_t Aggregater::getMsgId(Client* client, uint16_t clientMsgId)
{
	return _msgIdTable.getMsgId(client, clientMsgId);
}

void Aggregater::removeAggregateTopic(Topic* topic, Client* client)
{
      // ToDo: AggregateGW this method called when the client disconnect and erase it`s Topics. this method call */
}

void Aggregater::removeAggregateTopicList(Topics* topics, Client* client)
{
      // ToDo: AggregateGW this method called when the client disconnect and erase it`s Topics. this method call */
}

int Aggregater::addAggregateTopic(Topic* topic, Client* client)
{
	// ToDo: AggregateGW  */
	return 0;
}

AggregateTopicElement* Aggregater::createClientList(Topic* topic)
{
	// ToDo: AggregateGW  */
	return 0;
}

bool Aggregater::testMessageIdTable(void)
{
	Client* client = new Client();
	uint16_t msgId = 0;

	printf("msgId=%d\n", addMessageIdTable(client,1));
	printf("msgId=%d\n", addMessageIdTable(client,2));
	printf("msgId=%d\n", addMessageIdTable(client,3));
	printf("msgId=%d\n", addMessageIdTable(client,1));
	printf("msgId=%d\n", addMessageIdTable(client,2));
	printf("msgId=%d\n", addMessageIdTable(client,3));
	printf("msgId=%d\n", addMessageIdTable(client,4));
	printf("msgId=%d\n", addMessageIdTable(client,4));
	printf("msgId=%d\n", addMessageIdTable(client,4));

	convertClient(1,&msgId);
	printf("msgId=%d\n",msgId);
	convertClient(2,&msgId);
	printf("msgId=%d\n",msgId);
	convertClient(5,&msgId);
	printf("msgId=%d\n",msgId);
	convertClient(4,&msgId);
	printf("msgId=%d\n",msgId);
	convertClient(3,&msgId);
	printf("msgId=%d\n",msgId);
			return true;
}


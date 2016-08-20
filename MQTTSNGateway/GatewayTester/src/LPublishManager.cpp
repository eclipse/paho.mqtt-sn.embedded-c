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

#include <stdlib.h>
#include <string.h>

#include "LMqttsnClientApp.h"
#include "LTimer.h"
#include "LGwProxy.h"
#include "LMqttsnClient.h"
#include "LPublishManager.h"
#include "LScreen.h"

using namespace std;
using namespace linuxAsyncClient;

extern void setUint16(uint8_t* pos, uint16_t val);
extern uint16_t getUint16(const uint8_t* pos);
extern LMqttsnClient* theClient;
extern bool theOTAflag;
extern LScreen* theScreen;
/*========================================
 Class PublishManager
 =======================================*/
const char* NULLCHAR = "";

LPublishManager::LPublishManager()
{
	_first = 0;
	_elmCnt = 0;
	_publishedFlg = SAVE_TASK_INDEX;
}

LPublishManager::~LPublishManager()
{
	PubElement* elm = _first;
	PubElement* sav = 0;
	while (elm)
	{
		sav = elm->next;
		if (elm != 0)
		{
			delElement(elm);
		}
		elm = sav;
	}
}

void LPublishManager::publish(const char* topicName, Payload* payload, uint8_t qos, bool retain)
{
	publish(topicName, payload->getRowData(), payload->getLen(), qos, retain);
}


void LPublishManager::publish(const char* topicName, uint8_t* payload, uint16_t len, uint8_t qos, bool retain)
{
	uint8_t topicType = MQTTSN_TOPIC_TYPE_NORMAL;
	if ( strlen(topicName) < 2 )
	{
		topicType = MQTTSN_TOPIC_TYPE_SHORT;
	}
	publish(topicName, payload, len, qos, topicType, retain);
}

void LPublishManager::publish(const char* topicName, uint8_t* payload, uint16_t len, uint8_t qos, uint8_t topicType, bool retain)
{
	uint16_t msgId = 0;
	if ( qos > 0 )
	{
		msgId = theClient->getGwProxy()->getNextMsgId();
	}
	PubElement* elm = add(topicName, 0, payload, len, qos, retain, msgId, topicType);
	if (elm->status == TOPICID_IS_READY)
	{
		sendPublish(elm);
	}
	else
	{
		theClient->getGwProxy()->registerTopic((char*) topicName, 0);
	}
}

void LPublishManager::publish(uint16_t topicId, Payload* payload, uint8_t qos, bool retain)
{
	publish(topicId, payload->getRowData(), payload->getLen(), qos, retain);
}

void LPublishManager::publish(uint16_t topicId, uint8_t* payload, uint16_t len, uint8_t qos, bool retain)
{
	publish(topicId, payload, len, qos, retain);
}

void LPublishManager::publish(uint16_t topicId, uint8_t* payload, uint16_t len, uint8_t qos, uint8_t topicType, bool retain)
{
	uint16_t msgId = 0;
	if ( qos > 0 )
	{
		msgId = theClient->getGwProxy()->getNextMsgId();
	}
	PubElement* elm = add(NULLCHAR, topicId, payload, len, qos, retain, msgId, topicType);
	sendPublish(elm);
}

void LPublishManager::sendPublish(PubElement* elm)
{
	if (elm == 0)
	{
		return;
	}

	theClient->getGwProxy()->connect();

	uint8_t msg[MQTTSN_MAX_MSG_LENGTH + 1];
	uint8_t org = 0;
	if (elm->payloadlen > 128)
	{
		msg[0] = 0x01;
		setUint16(msg + 1, elm->payloadlen + 9);
		org = 2;
	}
	else
	{
		msg[0] = (uint8_t) elm->payloadlen + 7;
	}
	msg[org + 1] = MQTTSN_TYPE_PUBLISH;
	msg[org + 2] = elm->flag;
	if ((elm->retryCount < MQTTSN_RETRY_COUNT))
	{
		msg[org + 2] = msg[org + 2] | MQTTSN_FLAG_DUP;
	}
	if ((elm->flag & 0x03) == MQTTSN_TOPIC_TYPE_SHORT )
	{
		memcpy(msg + org + 3, elm->topicName, 2);
	}
	else
	{
		setUint16(msg + org + 3, elm->topicId);
	}
	setUint16(msg + org + 5, elm->msgId);
	memcpy(msg + org + 7, elm->payload, elm->payloadlen);

	theClient->getGwProxy()->writeMsg(msg);
	theClient->getGwProxy()->resetPingReqTimer();
	if ((elm->flag & 0x60) == MQTTSN_FLAG_QOS_0)
	{
		ASSERT("\033[0m\033[0;32m Topic \"%s\" was Published. \033[0m\033[0;37m\n\n", elm->topicName);
		remove(elm);  // PUBLISH Done
		return;
	}
	else if ((elm->flag & 0x60) == MQTTSN_FLAG_QOS_1)
	{
		elm->status = WAIT_PUBACK;
	}
	else if ((elm->flag & 0x60) == MQTTSN_FLAG_QOS_2)
	{
		elm->status = WAIT_PUBREC;
	}

	elm->sendUTC = LTimer::getUnixTime();
	elm->retryCount--;
}

void LPublishManager::sendSuspend(const char* topicName, uint16_t topicId, uint8_t topicType)
{
	PubElement* elm = _first;
	while (elm)
	{
		if (strcmp(elm->topicName, topicName) == 0 && elm->status == TOPICID_IS_SUSPEND)
		{
			elm->topicId = topicId;
			elm->flag |= topicType;
			elm->status = TOPICID_IS_READY;
			sendPublish(elm);
			elm = 0;
		}
		else
		{
			elm = elm->next;
		}
	}
}

void LPublishManager::sendPubAck(uint16_t topicId, uint16_t msgId, uint8_t rc)
{
	uint8_t msg[7];
	msg[0] = 7;
	msg[1] = MQTTSN_TYPE_PUBACK;
	setUint16(msg + 2, topicId);
	setUint16(msg + 4, msgId);
	msg[6] = rc;
	theClient->getGwProxy()->writeMsg(msg);
}

void LPublishManager::sendPubRel(PubElement* elm)
{
	uint8_t msg[4];
	msg[0] = 4;
	msg[1] = MQTTSN_TYPE_PUBREL;
	setUint16(msg + 2, elm->msgId);
	theClient->getGwProxy()->writeMsg(msg);
}

bool LPublishManager::isDone(void)
{
	return (_first == 0);
}

bool LPublishManager::isMaxFlight(void)
{
	return (_elmCnt > MAX_INFLIGHT_MSG / 2);
}

void LPublishManager::responce(const uint8_t* msg, uint16_t msglen)
{
	if (msg[0] == MQTTSN_TYPE_PUBACK)
	{
		uint16_t msgId = getUint16(msg + 3);
		PubElement* elm = getElement(msgId);
		if (elm == 0)
		{
			return;
		}
		if (msg[5] == MQTTSN_RC_ACCEPTED)
		{
			if (elm->status == WAIT_PUBACK)
			{
				ASSERT("\033[0m\033[0;32m Topic \"%s\" was Published. \033[0m\033[0;37m\n\n", elm->topicName);
				remove(elm); // PUBLISH Done
			}
		}
		else if (msg[5] == MQTTSN_RC_REJECTED_INVALID_TOPIC_ID)
		{
			elm->status = TOPICID_IS_SUSPEND;
			elm->topicId = 0;
			elm->retryCount = MQTTSN_RETRY_COUNT;
			elm->sendUTC = 0;
			theClient->getGwProxy()->registerTopic((char*) elm->topicName, 0);
		}
	}
	else if (msg[0] == MQTTSN_TYPE_PUBREC)
	{
		PubElement* elm = getElement(getUint16(msg + 1));
		if (elm == 0)
		{
			return;
		}
		if (elm->status == WAIT_PUBREC || elm->status == WAIT_PUBCOMP)
		{
			sendPubRel(elm);
			elm->status = WAIT_PUBCOMP;
			elm->sendUTC = LTimer::getUnixTime();
		}
	}
	else if (msg[0] == MQTTSN_TYPE_PUBCOMP)
	{
		PubElement* elm = getElement(getUint16(msg + 1));
		if (elm == 0)
		{
			return;
		}
		if (elm->status == WAIT_PUBCOMP)
		{
			ASSERT("\033[0m\033[0;32m Topic \"%s\" was Published. \033[0m\033[0;37m\n\n", elm->topicName);
			remove(elm);  // PUBLISH Done
		}
	}
}

void LPublishManager::published(uint8_t* msg, uint16_t msglen)
{
	uint16_t topicId = getUint16(msg + 2);

	if (msg[1] & MQTTSN_FLAG_QOS_1)
	{
		sendPubAck(topicId, getUint16(msg + 4), MQTTSN_RC_ACCEPTED);
	}

	if ((msg[1] & 0x03) == MQTTSN_TOPIC_TYPE_PREDEFINED)
	{
		if (topicId == 0x0001)
		{
			theOTAflag = true;
		}
	}
	else
	{
		_publishedFlg = NEG_TASK_INDEX;
		theClient->getTopicTable()->execCallback(topicId, msg + 6, msglen - 6, msg[1] & 0x03);
		_publishedFlg = SAVE_TASK_INDEX;
	}
}

void LPublishManager::checkTimeout(void)
{
	PubElement* elm = _first;
	while (elm)
	{
		if (elm->sendUTC > 0 && elm->sendUTC + MQTTSN_TIME_RETRY < LTimer::getUnixTime())
		{
			if (elm->retryCount >= 0)
			{
				sendPublish(elm);
				D_MQTTLOG("...Timeout retry\r\n");
			}
			else
			{
				theClient->getGwProxy()->reconnect();
				elm->retryCount = MQTTSN_RETRY_COUNT;
				break;
			}
		}
		elm = elm->next;
	}
}

PubElement* LPublishManager::getElement(uint16_t msgId)
{
	PubElement* elm = _first;
	while (elm)
	{
		if (elm->msgId == msgId)
		{
			return elm;
		}
		else
		{
			elm = elm->next;
		}
	}
	return 0;
}

PubElement* LPublishManager::getElement(const char* topicName)
{
	PubElement* elm = _first;
	while (elm)
	{
		if (strcmp(elm->topicName, topicName) == 0)
		{
			return elm;
		}
		else
		{
			elm = elm->next;
		}
	}
	return 0;
}

void LPublishManager::remove(PubElement* elm)
{
	if (elm)
	{
		if (elm->prev == 0)
		{
			_first = elm->next;
			if (elm->next != 0)
			{
				elm->next->prev = 0;
			}
		}
		else
		{
			elm->prev->next = elm->next;
		}
		delElement(elm);
		_elmCnt--;
	}
}

void LPublishManager::delElement(PubElement* elm)
{
	if (elm->taskIndex >= 0)
	{
		theClient->getTaskManager()->done(elm->taskIndex);
	}
	free(elm->payload);
	free(elm);
}

/*
 PubElement* PublishManager::add(const char* topicName, uint16_t topicId, MQTTSNPayload* payload, uint8_t qos, uint8_t retain, uint16_t msgId){
 return add(topicName, topicId, payload->getRowData(), payload->getLen(), qos, retain, msgId);
 }*/

PubElement* LPublishManager::add(const char* topicName, uint16_t topicId, uint8_t* payload, uint16_t len, uint8_t qos,
		uint8_t retain, uint16_t msgId, uint8_t topicType)
{
	PubElement* last = _first;
	PubElement* prev = _first;
	PubElement* elm = (PubElement*) calloc(1, sizeof(PubElement));

	if (elm == 0)
	{
		return 0;
	}
	if (last == 0)
	{
		_first = elm;
	}

	elm->topicName = topicName;
	elm->flag |= topicType;

	if (qos == 0)
	{
		elm->flag |= MQTTSN_FLAG_QOS_0;
	}
	else if (qos == 1)
	{
		elm->flag |= MQTTSN_FLAG_QOS_1;
	}
	else if (qos == 2)
	{
		elm->flag |= MQTTSN_FLAG_QOS_2;
	}
	if (retain)
	{
		elm->flag |= MQTTSN_FLAG_RETAIN;
	}

	if (topicId)
	{
		elm->status = TOPICID_IS_READY;
		elm->topicId = topicId;
	}

	elm->payload = (uint8_t*) malloc(len);
	if (elm->payload == 0)
	{
		return 0;
	}
	memcpy(elm->payload, payload, len);
	elm->payloadlen = len;
	elm->msgId = msgId;
	elm->retryCount = MQTTSN_RETRY_COUNT;
	elm->sendUTC = 0;

	if (_publishedFlg == NEG_TASK_INDEX)
	{
		elm->taskIndex = -1;
	}
	else
	{
		elm->taskIndex = theClient->getTaskManager()->getIndex();
		theClient->getTaskManager()->suspend(elm->taskIndex);
	}

	while (last)
	{
		prev = last;
		if (prev->next != 0)
		{
			last = prev->next;
		}
		else
		{
			prev->next = elm;
			elm->prev = prev;
			elm->next = 0;
			last = 0;
		}
	}
	++_elmCnt;
	return elm;
}

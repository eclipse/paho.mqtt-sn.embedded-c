/**************************************************************************************
 * Copyright (c) 2018, Tomoaki Yamaguchi
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

#ifndef MQTTSNGATEWAY_SRC_MQTTSNGWAGGREGATER_H_
#define MQTTSNGATEWAY_SRC_MQTTSNGWAGGREGATER_H_

#include "MQTTSNGWAdapter.h"
#include "MQTTSNGWMessageIdTable.h"
#include "MQTTSNGWAggregateTopicTable.h"
namespace MQTTSNGW
{
class Gateway;
class Adapter;
class Client;
class SensorNetAddress;
class MessageIdTable;
class AggregateTopicTable;
class Topics;

/*=====================================
     Class Aggregater
 =====================================*/
class Aggregater : public Adapter
{
	friend class MessageIdTable;
public:
    Aggregater(Gateway* gw);
    ~Aggregater(void);

    void initialize(void);

    const char* getClientId(SensorNetAddress* addr);
	Client* getClient(SensorNetAddress* addr);
	Client* convertClient(uint16_t msgId, uint16_t* clientMsgId);
	uint16_t addMessageIdTable(Client* client, uint16_t msgId);
	uint16_t getMsgId(Client* client, uint16_t clientMsgId);


	AggregateTopicElement* createClientList(Topic* topic);
	int addAggregateTopic(Topic* topic, Client* client);
	void removeAggregateTopic(Topic* topic, Client* client);
	void removeAggregateTopicList(Topics* topics, Client* client);
	bool isActive(void);

	bool testMessageIdTable(void);

private:
	uint16_t msgId(void);
    Gateway* _gateway {nullptr};
    MessageIdTable _msgIdTable;
    AggregateTopicTable _topicTable;

    bool _isActive {false};
    bool _isSecure {false};
};



}



#endif /* MQTTSNGATEWAY_SRC_MQTTSNGWAGGREGATER_H_ */

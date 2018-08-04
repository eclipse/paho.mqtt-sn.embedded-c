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

#ifndef MQTTSNGATEWAY_SRC_MQTTSNGWAGGREGATETOPICTABLE_H_
#define MQTTSNGATEWAY_SRC_MQTTSNGWAGGREGATETOPICTABLE_H_

#include "MQTTSNGWDefines.h"
#include "MQTTSNGWProcess.h"
#include <stdint.h>
namespace MQTTSNGW
{

class Client;
class Topic;
class AggregateTopicElement;
class ClientTopicElement;
class Mutex;

/*=====================================
 Class AggregateTopicTable
 ======================================*/
class AggregateTopicTable
{
public:
	AggregateTopicTable();
	~AggregateTopicTable();

	AggregateTopicElement* add(Topic* topic, Client* client);
	AggregateTopicElement* getClientList(Topic* client);
	void remove(Topic* topic, Client* client);
	void clear(void);
private:
	AggregateTopicElement* _head {nullptr};
	AggregateTopicElement* _tail {nullptr};
	int _cnt {0};
	int _maxSize {MAX_MESSAGEID_TABLE_SIZE};
};

/*=====================================
 Class AggregateTopicElement
 =====================================*/
class AggregateTopicElement
{
    friend class AggregateTopicTable;
public:
    AggregateTopicElement(void);
    AggregateTopicElement(Topic* topic, Client* client);
    ~AggregateTopicElement(void);

    ClientTopicElement* add(Client* client);
    ClientTopicElement* getFirstElement(void);
    ClientTopicElement* getNextElement(ClientTopicElement* elm);
    void erase(ClientTopicElement* elm);
    ClientTopicElement* find(Client* client);

private:
    Mutex _mutex;
    Topic* _topic {nullptr};
    ClientTopicElement* _head {nullptr};
    ClientTopicElement* _tail {nullptr};
};

/*=====================================
 Class ClientTopicElement
 =====================================*/
class ClientTopicElement
{
    friend class AggregateTopicTable;
    friend class AggregateTopicElement;
public:
    ClientTopicElement(Client* client);
    ~ClientTopicElement(void);
    Client* getClient(void);

private:
    Client* _client {nullptr};
    ClientTopicElement* _next {nullptr};
    ClientTopicElement* _prev {nullptr};
};

}



#endif /* MQTTSNGATEWAY_SRC_MQTTSNGWAGGREGATETOPICTABLE_H_ */

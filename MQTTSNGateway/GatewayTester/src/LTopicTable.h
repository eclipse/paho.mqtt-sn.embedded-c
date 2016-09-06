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

#ifndef TOPICTABLE_H_
#define TOPICTABLE_H_

#include <stdio.h>

#include "LMqttsnClientApp.h"
#include "Payload.h"

#define MQTTSN_TOPIC_MULTI_WILDCARD   1
#define MQTTSN_TOPIC_SINGLE_WILDCARD  2

namespace linuxAsyncClient {
/*=====================================
        Class LTopic
 ======================================*/
typedef int (*TopicCallback)(uint8_t*, uint16_t);

class LTopic {
	friend class LTopicTable;
public:
    LTopic();
    ~LTopic();
    int      execCallback(uint8_t* payload, uint16_t payloadlen);
    uint8_t  hasWildCard(uint8_t* pos);
    bool     isMatch(const char* topic);
    TopicCallback getCallback(void);
private:
    uint16_t  _topicId;
    uint8_t   _topicType;
    char*     _topicStr;
    TopicCallback  _callback;
    uint8_t  _malocFlg;
    LTopic*    _prev;
    LTopic*    _next;
};

/*=====================================
        Class LTopicTable
 ======================================*/
class LTopicTable {
public:
	LTopicTable();
      ~LTopicTable();
      uint16_t getTopicId(const char* topic);
      char*    getTopicName(LTopic* topic);
      LTopic*   getTopic(const char* topic);
      LTopic*   getTopic(uint16_t topicId, uint8_t topicType = MQTTSN_TOPIC_TYPE_NORMAL);
      void     setTopicId(const char* topic, uint16_t id, uint8_t topicType);
      bool     setCallback(const char* topic, TopicCallback callback);
      bool     setCallback(uint16_t topicId, uint8_t type, TopicCallback callback);
      int      execCallback(uint16_t topicId, uint8_t* payload, uint16_t payloadlen, uint8_t topicType = MQTTSN_TOPIC_TYPE_NORMAL);
      LTopic*  add(const char* topic, uint16_t id = 0, uint8_t type = MQTTSN_TOPIC_TYPE_NORMAL, TopicCallback callback = 0, uint8_t alocFlg = 0);
      LTopic*  add(uint16_t topicId, uint16_t id, uint8_t type, TopicCallback callback, uint8_t alocFlg);
      LTopic*  match(const char* topic);
      void     clearTopic(void);
      void     remove(uint16_t topicId);

private:
    LTopic*  _first;
    LTopic*  _last;

};

} /* end of namespace */

#endif /* TOPICTABLE_H_ */

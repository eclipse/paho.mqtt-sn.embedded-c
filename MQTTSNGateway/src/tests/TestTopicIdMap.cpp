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
 *    Tomoaki Yamaguchi - initial API and implementation 
 **************************************************************************************/
#include <stdlib.h>
#include <string.h>
#include <cassert>
#include "TestTopicIdMap.h"

using namespace std;
using namespace MQTTSNGW;

TestTopicIdMap::TestTopicIdMap()
{
	_map = new TopicIdMap();
}

TestTopicIdMap::~TestTopicIdMap()
{
	delete _map;
}

#define MAXID 30

void TestTopicIdMap::test(void)
{
	uint16_t id[MAXID];
	printf("Test TopicIdMat start.\n");

	for ( int i = 0; i < MAXID; i++ )
	{
		id[i] = i + 1;
	}

	for ( int i = 0; i < MAXID; i++ )
	{
		_map->add(id[i], id[i], MQTTSN_TOPIC_TYPE_NORMAL);
	}

	for ( int i = 0; i < MAXID; i++ )
	{
		MQTTSN_topicTypes type = MQTTSN_TOPIC_TYPE_SHORT;
		uint16_t topicId = _map->getTopicId((uint16_t)i, &type);
		//printf("TopicId=%d  msgId=%d type=%d\n", topicId, i, type);
		assert((i <= MAX_INFLIGHTMESSAGES * 2 + 1 && topicId == i) || (i > MAX_INFLIGHTMESSAGES * 2 + 1 && topicId == 0));
	}

	//printf("\n");

	for ( int i = 0; i < 5; i++ )
	{
		_map->erase(i);
	}
	for ( int i = 0; i < MAXID; i++ )
	{
		MQTTSN_topicTypes type = MQTTSN_TOPIC_TYPE_SHORT;
		uint16_t topicId = _map->getTopicId((uint16_t)i, &type);
		//printf("TopicId=%d  msgId=%d type=%d\n", topicId, i, type);
		assert((i < 5 && topicId == 0) || (i >= 5 && topicId != 0) || (i > MAX_INFLIGHTMESSAGES * 2 + 1 && topicId == 0) );
	}


	_map->clear();
	//printf("\n");

	for ( int i = 0; i < MAXID; i++ )
	{
		MQTTSN_topicTypes type = MQTTSN_TOPIC_TYPE_SHORT;
		uint16_t topicId = _map->getTopicId((uint16_t)i, &type);
		//printf("TopicId=%d  msgId=%d type=%d\n", topicId, i, type);
		assert( topicId == 0 );
	}

	for ( int i = 0; i < MAXID; i++ )
	{
		_map->add(id[i], id[i], MQTTSN_TOPIC_TYPE_SHORT);
	}

	for ( int i = 0; i < MAXID; i++ )
	{
		MQTTSN_topicTypes type = MQTTSN_TOPIC_TYPE_NORMAL;
		uint16_t topicId = _map->getTopicId((uint16_t)i, &type);
		//printf("TopicId=%d  msgId=%d type=%d\n", topicId, i, type);
		assert((i <= MAX_INFLIGHTMESSAGES * 2 + 1 && topicId == i) || (i > MAX_INFLIGHTMESSAGES * 2 + 1 && topicId == 0));
	}

	//printf("\n");

	for ( int i = 0; i < 5; i++ )
	{
		_map->erase(i);
	}
	for ( int i = 0; i < MAXID; i++ )
	{
		MQTTSN_topicTypes type = MQTTSN_TOPIC_TYPE_NORMAL;
		uint16_t topicId = _map->getTopicId((uint16_t)i, &type);
		//printf("TopicId=%d  msgId=%d type=%d\n", topicId, i, type);
		assert((i < 5 && topicId == 0) || (i >= 5 && topicId != 0) || (i > MAX_INFLIGHTMESSAGES * 2 + 1 && topicId == 0) );
	}


	_map->clear();

	//printf("\n");
	for ( int i = 0; i < MAXID; i++ )
	{
		MQTTSN_topicTypes type = MQTTSN_TOPIC_TYPE_NORMAL;
		uint16_t topicId = _map->getTopicId((uint16_t)i, &type);
		//printf("TopicId=%d  msgId=%d type=%d\n", topicId, i, type);
		assert( topicId == 0 );
	}
	printf("Test TopicIdMat completed.\n\n");
}


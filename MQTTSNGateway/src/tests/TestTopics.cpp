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
#include "TestTopics.h"

using namespace std;
using namespace MQTTSNGW;

TestTopics::TestTopics()
{
	_topics = new Topics();
}

TestTopics::~TestTopics()
{
	delete _topics;
}

void TestTopics::test(void)
{
	printf("Test  Topics         ");

	MQTTSN_topicid topic[12];
	char tp[12][10];

	/* create Topic */
	strcpy(tp[0], "Topic/+");
	tp[0][7] = 0;
	topic[0].type = MQTTSN_TOPIC_TYPE_NORMAL;
	topic[0].data.long_.len = strlen(tp[0]);
	topic[0].data.long_.name = tp[0];

	for ( int i = 1; i < 10 ; i++ )
	{
		strcpy(tp[i], "Topic/+/");
		tp[i][8] = 0x30 + i;
		tp[i][9] = 0;
		topic[i].type = MQTTSN_TOPIC_TYPE_NORMAL;
		topic[i].data.long_.len = strlen(tp[i]);
		topic[i].data.long_.name = tp[i];
	}
	strcpy(tp[10], "TOPIC/#");
	tp[10][7] = 0;
	topic[10].type = MQTTSN_TOPIC_TYPE_NORMAL;
	topic[10].data.long_.len = strlen(tp[10]);
	topic[10].data.long_.name = tp[10];

	strcpy(tp[11], "+/0/#");
	tp[11][7] = 0;
	topic[11].type = MQTTSN_TOPIC_TYPE_NORMAL;
	topic[11].data.long_.len = strlen(tp[11]);
	topic[11].data.long_.name = tp[11];

	/* Add Topic to Topics */
	for ( int i = 0; i < 12; i++ )
	{
		MQTTSN_topicid pos = topic[i];
		Topic* t = _topics->add(&pos);
		//printf("Topic=%s ID=%d\n", t->getTopicName()->c_str(), t->getTopicId());
		assert(t !=0);
	}

	for ( int i = 0; i < 5; i++ )
	{
		string str = "Test/";
		str += 0x30 + i;
		Topic* t = _topics->add(&str);
		//printf("Topic=%s ID=%d\n", t->getTopicName()->c_str(), t->getTopicId());
		assert(t !=0);
	}

	/* Get Topic by MQTTSN_topicid */
	for ( int i = 0; i < 12; i++ )
	{
		Topic* t = _topics->getTopic(&topic[i]);
		//printf("Topic=%s ID=%d ID=%d\n", t->getTopicName()->c_str(), t->getTopicId(),_topics->getTopicId(&topic[i]));
		assert(t->getTopicId() == i + 1);
	}

	/* Get TopicId by MQTTSN_topicid */
	for ( int i = 0; i < 12; i++ )
	{
		uint16_t id = _topics->getTopicId(&topic[i]);
		//printf("ID=%d \n", id);
		assert(id == i + 1);
	}

	/* Test Wilecard */
	for ( int i = 0; i < 10 ; i++ )
	{
		MQTTSN_topicid tp1;
		char tp0[10];
		strcpy(tp0, "Topic/");
		tp0[6] = 0x30 + i;
		tp0[7] = '/';
		tp0[8] = 0x30 + i;
		tp0[9] = 0;
		tp1.type = MQTTSN_TOPIC_TYPE_NORMAL;
		tp1.data.long_.len = strlen(tp0);
		tp1.data.long_.name = tp0;

		Topic* t = _topics->match(&tp1);
/*
		if (t)
		{
			printf("Topic=%s  match to %s\n", tp0, t->getTopicName()->c_str());
		}
		else
		{
			printf("Topic=%s unmatch\n", tp0);
		}
*/
		assert(t != 0);
	}
	for ( int i = 0; i < 10 ; i++ )
	{
		MQTTSN_topicid tp1;
		char tp0[10];
		strcpy(tp0, "Topic/");
		tp0[6] = 0x30 + i;
		tp0[7] = 0;
		tp1.type = MQTTSN_TOPIC_TYPE_NORMAL;
		tp1.data.long_.len = strlen(tp0);
		tp1.data.long_.name = tp0;

		Topic* t = _topics->match(&tp1);
/*
		if (t)
		{
			printf("Topic=%s  match to %s\n", tp0, t->getTopicName()->c_str());
		}
		else
		{
			printf("Topic=%s unmatch\n", tp0);
		}
*/
		assert(t != 0);
	}

	for ( int i = 0; i < 10 ; i++ )
	{
		MQTTSN_topicid tpid1;
		char tp0[10];
		strcpy(tp0, "TOPIC/");
		tp0[6] = 0x30 + i;
		tp0[7] = '/';
		tp0[8] = 0x30 + i;
		tp0[9] = 0;
		tpid1.type = MQTTSN_TOPIC_TYPE_NORMAL;
		tpid1.data.long_.len = strlen(tp0);
		tpid1.data.long_.name = tp0;

		Topic* t = _topics->match(&tpid1);
/*
		if (t)
		{
			printf("Topic=%s  match to %s\n", tp0, t->getTopicName()->c_str());
		}
		else
		{
			printf("Topic=%s unmatch\n", tp0);
		}
*/
		assert( t != 0);
	}
	printf("[ OK ]\n");
}

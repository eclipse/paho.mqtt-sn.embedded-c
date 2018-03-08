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
 *    Tieto Poland Sp. z o.o. - Topic test improvements
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

bool testIsMatch(const char* topicFilter, const char* topicName)
{
	string* filter = new string(topicFilter);
	string* name = new string(topicName);

	Topic topic(filter);
	bool isMatch = topic.isMatch(name);

	delete name;

	return isMatch;
}

bool testGetTopic(const char* topicName, const char* searchedTopicName)
{
	Topics topics;
	string name(topicName);
	MQTTSN_topicid topicid;
	topicid.type = MQTTSN_TOPIC_TYPE_NORMAL;
	topicid.data.long_.len = strlen(searchedTopicName);
	topicid.data.long_.name = const_cast<char*>(searchedTopicName);

	topics.add(&name);

	return topics.getTopic(&topicid) != 0;
}

bool testGetTopicId(const char* topicName, const char* searchedTopicName)
{
	Topics topics;
	string name(topicName);
	MQTTSN_topicid topicid;
	topicid.type = MQTTSN_TOPIC_TYPE_NORMAL;
	topicid.data.long_.len = strlen(searchedTopicName);
	topicid.data.long_.name = const_cast<char*>(searchedTopicName);

	topics.add(&name);

	return topics.getTopicId(&topicid) != 0;
}

void TestTopics::test(void)
{
	printf("Test  Topics         ");

	const int TOPIC_COUNT = 13;

	MQTTSN_topicid topic[TOPIC_COUNT];
	char tp[TOPIC_COUNT][10];

	/* create Topic */
	strcpy(tp[0], "Topic/+");
	tp[0][7] = 0;
	topic[0].type = MQTTSN_TOPIC_TYPE_NORMAL;
	topic[0].data.long_.len = strlen(tp[0]);
	topic[0].data.long_.name = tp[0];

	for ( int i = 1; i < 10 ; i++ )
	{
		sprintf(tp[i], "Topic/+/%d", i);
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

	tp[12][0] = '#';
	tp[12][1] = 0;
	topic[12].type = MQTTSN_TOPIC_TYPE_NORMAL;
	topic[12].data.long_.len = strlen(tp[12]);
	topic[12].data.long_.name = tp[12];

	/* Add Topic to Topics */
	for ( int i = 0; i < TOPIC_COUNT; i++ )
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
	for ( int i = 0; i < TOPIC_COUNT; i++ )
	{
		Topic* t = _topics->getTopic(&topic[i]);
		//printf("Topic=%s ID=%d ID=%d\n", t->getTopicName()->c_str(), t->getTopicId(),_topics->getTopicId(&topic[i]));
		assert(t->getTopicId() == i + 1);
	}

	/* Get TopicId by MQTTSN_topicid */
	for ( int i = 0; i < TOPIC_COUNT; i++ )
	{
		uint16_t id = _topics->getTopicId(&topic[i]);
		//printf("ID=%d \n", id);
		assert(id == i + 1);
	}

	/* Test Wildcard */
	for ( int i = 0; i < 10 ; i++ )
	{
		MQTTSN_topicid tp1;
		char tp0[10];
		sprintf(tp0, "Topic/%d/%d", i, i);
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
		sprintf(tp0, "Topic/%d", i);
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
		assert(t->getTopicName()->compare(tp[0]) == 0);
	}

	for ( int i = 0; i < 10 ; i++ )
	{
		MQTTSN_topicid tpid1;
		char tp0[10];
		sprintf(tp0, "TOPIC/%d/%d", i, i);
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
		assert(t->getTopicName()->compare(tp[10]) == 0);
	}

	{
		MQTTSN_topicid tp1;
		char tp0[10];
		strcpy(tp0, "Topic");
		tp1.type = MQTTSN_TOPIC_TYPE_NORMAL;
		tp1.data.long_.len = strlen(tp0);
		tp1.data.long_.name = tp0;

		Topic* t = _topics->match(&tp1);

		assert(t != 0);
		assert(t->getTopicName()->compare(tp[12]) == 0);
	}

	{
		MQTTSN_topicid tp1;
		char tp0[20];
		strcpy(tp0, "Topic/multi/level");
		tp1.type = MQTTSN_TOPIC_TYPE_NORMAL;
		tp1.data.long_.len = strlen(tp0);
		tp1.data.long_.name = tp0;

		Topic* t = _topics->match(&tp1);

		assert(t != 0);
		assert(t->getTopicName()->compare(tp[12]) == 0);
	}


	assert(testIsMatch("#", "one"));
	assert(testIsMatch("#", "one/"));
	assert(testIsMatch("#", "one/two"));
	assert(testIsMatch("#", "one/two/"));
	assert(testIsMatch("#", "one/two/three"));
	assert(testIsMatch("#", "one/two/three/"));

	assert(!testIsMatch("one/+", "one"));
	assert(testIsMatch("one/+", "one/"));
	assert(testIsMatch("one/+", "one/two"));
	assert(!testIsMatch("one/+", "one/two/"));
	assert(!testIsMatch("one/+", "one/two/three"));

	assert(!testIsMatch("one/+/three/+", "one/two/three"));
	assert(testIsMatch("one/+/three/+", "one/two/three/"));
	assert(testIsMatch("one/+/three/+", "one/two/three/four"));
	assert(!testIsMatch("one/+/three/+", "one/two/three/four/"));

	assert(testIsMatch("one/+/three/#", "one/two/three"));
	assert(testIsMatch("one/+/three/#", "one/two/three/"));
	assert(testIsMatch("one/+/three/#", "one/two/three/four"));
	assert(testIsMatch("one/+/three/#", "one/two/three/four/"));
	assert(testIsMatch("one/+/three/#", "one/two/three/four/five"));

	// examples from MQTT specification
	assert(testIsMatch("sport/tennis/player1/#", "sport/tennis/player1"));
	assert(testIsMatch("sport/tennis/player1/#", "sport/tennis/player1/ranking"));
	assert(testIsMatch("sport/tennis/player1/#", "sport/tennis/player1/score/wimbledon"));
	assert(testIsMatch("sport/tennis/+", "sport/tennis/player1"));
	assert(testIsMatch("sport/tennis/+", "sport/tennis/player2"));
	assert(!testIsMatch("sport/tennis/+", "sport/tennis/player1/ranking"));
	assert(testIsMatch("+/+", "/finance"));
	assert(testIsMatch("/+", "/finance"));
	assert(!testIsMatch("+", "/finance"));

	assert(testGetTopicId("mytopic", "mytopic"));
	assert(!testGetTopicId("mytopic", "mytop"));
	assert(!testGetTopicId("mytopic", "mytopiclong"));

	assert(testGetTopic("mytopic", "mytopic"));
	assert(!testGetTopic("mytopic", "mytop"));
	assert(!testGetTopic("mytopic", "mytopiclong"));

	printf("[ OK ]\n");
}

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

#include <string.h>
#include <cassert>
#include "TestProcessFramework.h"
#include "MQTTSNGWProcess.h"
#include "Timer.h"

using namespace std;
using namespace MQTTSNGW;

#define ARGV    "./testPFW"
#define CONFDIR "./"
#define CONF    "gateway.conf"

const char* currentDateTime(void);

TestProcessFramework::TestProcessFramework()
{
	theMultiTaskProcess = this;
	theProcess = this;
}

TestProcessFramework::~TestProcessFramework()
{

}

void TestProcessFramework::initialize(int argc, char** argv)
{
	MultiTaskProcess::initialize(argc, argv);
	assert(0 == strcmp(CONFDIR, getConfigDirName()->c_str()));
	assert(0 == strcmp(CONF, getConfigFileName()->c_str()));
	resetRingBuffer();
}

void TestProcessFramework::run(void)
{
	char value[256];
	int* v = 0;
	int i = 0;
	Timer tm;
	TestQue que;

	assert(1 == getArgc() || 3 == getArgc() );
	assert(0 == strcmp(ARGV, *getArgv()));
	getParam("BrokerName", value);
	assert(0 == strcmp("iot.eclipse.org", value));

	for ( i = 0; i < 1000; i++)
	{
		putLog("Test RingBuffer %d ", 1234567890);
	}
	putLog("\n\nRingBuffer Test complieted. Enter CTRL+C\n");

	for ( i = 0; i < 10; i++ )
	{
		v = new int(i);
		que.post(v);
	}
	assert( 10 == que.size());

	for ( i = 0; i < 10; i++ )
	{
		assert(i == *que.front());
		int* p = que.front();
		if ( p )
		{
			assert(i == *p);
			que.pop();
			delete p;
		}
	}
	assert(0 == que.front());
	assert(0 == que.size());

	que.setMaxSize(5);
	for ( i = 0; i < 10; i++ )
	{
		v = new int(i);
		que.post(v);
		assert( 5 >= que.size());
	}
	for ( i = 0; i < 10; i++ )
	{
		int* p = que.front();
		if ( p )
		{
			que.pop();
			delete p;
		}
	}

	printf("%s Timer start\n",  currentDateTime());
	tm.start(1000);
	while (!tm.isTimeup());
	printf("%s Timer 1sec\n", currentDateTime());

	tm.start();
	while (!tm.isTimeup(1000));
	printf("%s Timer 1sec\n", currentDateTime());

	MultiTaskProcess::run();

	printf("ProcessFramework test complited.\n");
}

TestQue::TestQue()
{

}

TestQue::~TestQue()
{

}

int* TestQue::front(void)
{
	return _que.front();
}
void TestQue::pop(void)
{
	_que.pop();
}
int TestQue::size(void)
{
	return _que.size();
}
void TestQue::setMaxSize(int maxsize)
{
	_que.setMaxSize(maxsize);
}

void TestQue::post(int* val)
{
	_que.post(val);
}

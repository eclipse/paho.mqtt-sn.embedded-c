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

#include "LMqttsnClientApp.h"
#include "LMqttsnClient.h"
#include "LScreen.h"

using namespace std;
using namespace linuxAsyncClient;
extern LMqttsnClient* theClient;
extern LScreen* theScreen;
extern int run(void);

/*
 *   Functions supported.
 *
 *   void PUBLISH    ( const char* topicName, uint8_t* payload,
 *                     uint16_t len, uint8_t qos, bool retain = false );
 *
 *   void PUBLISH    ( uint16_t topicId, uint8_t* payload,
 *                     uint16_t len, uint8_t qos, bool retain = false );
 *
 *   void SUBSCRIBE  ( const char* topicName, TopicCallback onPublish,
 *                      uint8_t qos );
 *
 *   void UNSUBSCRIBE( const char* topicName );
 *
 *   void DISCONNECT ( uint16_t sleepInSecs );
 *
 *   void DISPLAY( format, .....);    <== instead of printf()
 *
 */
/*------------------------------------------------------
 *    UDP Configuration
 *------------------------------------------------------*/
UDPCONF  = {
	"GatewayTester",     // ClientId
	{225,1,1,1},         // Multicast group IP
	1883,                // Multicast group Port
	20001,               // Local PortNo
};

/*------------------------------------------------------
 *    Client Configuration
 *------------------------------------------------------*/
MQTTSNCONF = {
	300,            //KeepAlive (seconds)
	true,           //Clean session
	0,              //Sleep duration in msecs
	"willTopic",    //WillTopic
	"willMessage",  //WillMessage
    0,              //WillQos
    false           //WillRetain
};

/*------------------------------------------------------
 *     Define Topics
 *------------------------------------------------------*/
const char* topic1 = "ty4tw/topic1";
const char* topic2 = "ty4tw/topic2";
const char* topic3 = "ty4tw/topic3";


/*------------------------------------------------------
 *       Callback routines for Subscribed Topics
 *------------------------------------------------------*/
int on_Topic01(uint8_t* pload, uint16_t ploadlen)
{
	DISPLAY("\n\nTopic1 recv.\n");
	char c = pload[ploadlen-1];
	pload[ploadlen-1]= 0;   // set null terminator
	DISPLAY("Payload -->%s%c<--\n\n",pload, c);
	return 0;
}

int on_Topic02(uint8_t* pload, uint16_t ploadlen)
{
	DISPLAY("\n\nTopic2 recv.\n");
	pload[ploadlen-1]= 0;   // set null terminator
	DISPLAY("Payload -->%s　<--\n\n",pload);
	return 0;
}

int on_Topic03(uint8_t* pload, uint16_t ploadlen)
{
	DISPLAY("\n\nNew callback recv Topic2\n");
	pload[ploadlen-1]= 0;   // set null terminator
	DISPLAY("Payload -->%s　<--\n\n",pload);
	return 0;
}

/*------------------------------------------------------
 *      A Link list of Callback routines and Topics
 *------------------------------------------------------*/

SUBSCRIBE_LIST = {// e.g. SUB(topic, callback, QoS),
				  SUB(topic1, on_Topic01, 1),
				  END_OF_SUBSCRIBE_LIST
				 };


/*------------------------------------------------------
 *    Test functions
 *------------------------------------------------------*/

void publishTopic1(void)
{
	char payload[300];
	sprintf(payload, "publish \"ty4tw/Topic1\" \n");
	uint8_t qos = 0;
	PUBLISH(topic1,(uint8_t*)payload, strlen(payload), qos);
}

void subscribeTopic2(void)
{
	uint8_t qos = 1;
	SUBSCRIBE(topic2, on_Topic02, qos);
}

void publishTopic2(void)
{
	char payload[300];
	sprintf(payload, "publish \"ty4tw/topic2\" \n");
	uint8_t qos = 0;
	PUBLISH(topic2,(uint8_t*)payload, strlen(payload), qos);
}

void unsubscribe(void)
{
	UNSUBSCRIBE(topic2);
}

void subscribechangeCallback(void)
{
	uint8_t qos = 1;
	SUBSCRIBE(topic2, on_Topic03, qos);
}

void test3(void)
{
	char payload[300];
	sprintf(payload, "TEST3 ");
	uint8_t qos = 0;
	PUBLISH(topic2,(uint8_t*)payload, strlen(payload), qos);
}

void disconnect(void)
{
	DISCONNECT(0);
}

/*------------------------------------------------------
 *    A List of Test functions
 *------------------------------------------------------*/

TEST_LIST = {// e.g. TEST( Label, Test),
			 TEST("Step1:Publish topic1",     publishTopic1),
			 TEST("Step2:Publish topic2",     publishTopic2),
			 TEST("Step3:Subscribe topic2",   subscribeTopic2),
			 TEST("Step4:Publish topic2",     publishTopic2),
			 TEST("Step5:Unsubscribe topic2", unsubscribe),
			 TEST("Step6:Publish topic2",     publishTopic2),
			 TEST("Step7:subscribe again",    subscribechangeCallback),
			 TEST("Step8:Publish topic2",     publishTopic2),
			 TEST("Step9:Disconnect",         disconnect),
			 END_OF_TEST_LIST
			};


/*------------------------------------------------------
 *    unused for Test
 *------------------------------------------------------*/
TASK_LIST = {// e.g. TASK( task, executing duration in second),
			//TASK(test1, 4);
             END_OF_TASK_LIST
            };

/*------------------------------------------------------
 *    Initialize function
 *------------------------------------------------------*/
void setup(void)
{

}

/*------------------------------------------------------
 *    main
 *------------------------------------------------------*/

int main(int argc, char** argv)
{
	return run();
}

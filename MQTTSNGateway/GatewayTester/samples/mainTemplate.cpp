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
 *   MQTT-SN Functions supported :
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
 *   void DISPLAY( format, valiables, .....);    <== instead of printf()
 *
 */

/*------------------------------------------------------
 *    UDP Configuration
 *------------------------------------------------------*/
UDPCONF  = {
	"GatewayTester",     // ClientId
	{225,1,1,1},         // Multicast group IP
	1883,                // Multicast group Port
	20000,               // Local PortNo
};

/*------------------------------------------------------
 *    Client Configuration
 *------------------------------------------------------*/
MQTTSNCONF = {
	60,             //KeepAlive (seconds)
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
const char* topic1 = "ty4tw/clientId";

/*------------------------------------------------------
 *       Callback routines for Subscribed Topics
 *------------------------------------------------------*/
int on_publish01(uint8_t* pload, uint16_t ploadlen)
{
	return 0;
}


/*------------------------------------------------------
 *      A Link list of Callback routines and Topics
 *------------------------------------------------------*/

SUBSCRIBE_LIST = {// e.g. SUB(topic, callback, QoS),
				  //SUB(topic1, on_publish01, 1),
				  END_OF_SUBSCRIBE_LIST
				 };

/*------------------------------------------------------
 *    Test functions
 *------------------------------------------------------*/
void test1(void)
{
	char payload[300];
	sprintf(payload, "Client-01 ");
	uint8_t qos = 0;
	PUBLISH(topic1,(uint8_t*)payload, strlen(payload), qos);
}

void test2(void)
{

}

void test3(void)
{

}

void test4(void)
{

}

void test5(void)
{

}

/*------------------------------------------------------
 *    A List of Test functions
 *------------------------------------------------------*/

TEST_LIST = {// e.g. TEST( Label, Test),
			 TEST("Publish topic1",     test1),
			 END_OF_TEST_LIST
			};


/*------------------------------------------------------
 *    unused for Test
 *------------------------------------------------------*/
TASK_LIST = {// e.g. TASK( task, executing duration in second),
			TASK(test1, 4),
             END_OF_TASK_LIST
            };


/*------------------------------------------------------
 *    Initialize function
 *------------------------------------------------------*/
void setup(void)
{

}

/*======================================================
 *    main
 *======================================================*/

/*   uncomment this

int main(int argc, char** argv)
{
	return run();
}

*/

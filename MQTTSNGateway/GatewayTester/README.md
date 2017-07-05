###Gateway Test Program.    
**sample/mainTest.cpp** is a Test sample coading.   
Each test is described as one function. test1(), test2()...     
````
/*------------------------------------------------------
 *    Test functions
 *
 *    you can use 4 commands in Test functions
 *
 *    1) PUBLISH(const char* topicName,
 *               uint8_t*    payload,
 *               uint16_t    len,
 *               uint8_t     qos,
 *               bool        retain = false);
 *
 *    2) SUBSCRIBE(const char*   topicName,
 *                 TopicCallback onPublish,
 *                 uint8_t qos);
 *
 *    3) UNSUBSCRIBE(const char* topicName);
 *
 *    4) DISCONNECT(uint16_t sleepInSecs);
 *
 *------------------------------------------------------*/

void test1(void)
{
	char payload[300];
	sprintf(payload, "ESP8266-08b133 ");
	uint8_t qos = 0;
	PUBLISH(topic1,(uint8_t*)payload, strlen(payload), qos);
}

void test2(void)
{
	uint8_t qos = 1;
	SUBSCRIBE(topic2, on_publish02, qos);
}
````
**TEST_LIST** is a test senario. Test functions are executed one by one. 
```` 
/*------------------------------------------------------
 *    A List of Test Tasks
 *------------------------------------------------------*/

TEST_LIST = {// e.g. TEST( Label, Test),
			 TEST("Publish topic1",     test1),
			 TEST("Subscribe topic2",   test2),
			 TEST("Publish topic2",     test3),
			 TEST("Unsubscribe topic2", test4),
			 TEST("Publish topic2",     test3),
			 TEST("Disconnect",         test5),
			 END_OF_TEST_LIST
			};
```` 

### **step1. Build **   
````
$ git clone https://github.com/eclipse/paho.mqtt-sn.embedded-c 
$ cd paho.mqtt-sn.embedded-c/MQTTSNGateway/GatewayTester       
$ make   
$ make install   
$ make clean
```       
MQTT-SNGatewayTester program is copied into ../../../ directory.

    
### **step2. Execute Gateway Tester.**     

````    
$ cd ../../..   
$ ./MQTT-SNGatewayTester
  
 ***************************************************************************
 * MQTT-SN Gateway Tester
 * Part of Project Paho in Eclipse
 * (http://git.eclipse.org/c/paho/org.eclipse.paho.mqtt-sn.embedded-c.git/)
 *
 * Author : Tomoaki YAMAGUCHI
 * Version: 0.0.0
 ***************************************************************************

Attempting to Connect the Broker.....

sendto 225.1.1.1      :1883   03 01 00

recved 192.168.11.5   :1883   03 01 00
sendto 225.1.1.1      :1883   03 01 00

recved 192.168.11.5   :1883   03 01 00

recved 192.168.11.17  :10000  03 02 01
sendto 192.168.11.17  :10000  13 04 0c 01 03 84 47 61 74 65 77 61 79 54 65 73 74 65 72

recved 192.168.11.17  :10000  02 06
sendto 192.168.11.17  :10000  0c 07 00 77 69 6c 6c 54 6f 70 69 63

recved 192.168.11.17  :10000  02 08
sendto 192.168.11.17  :10000  0d 09 77 69 6c 6c 4d 65 73 73 61 67 65

recved 192.168.11.17  :10000  03 05 00


 Connected to the Broker

 Attempting OnConnect.....
sendto 192.168.11.17  :10000  13 12 20 00 01 74 79 34 74 77 2f 63 6c 69 65 6e 74 49 64

recved 192.168.11.17  :10000  08 13 20 00 01 00 01 00


 SUBSCRIBE complete. ty4tw/clientId

 OnConnect complete
 Test Ready.

Execute Publish topic1 Test ? ( Y/N ) :  

````    

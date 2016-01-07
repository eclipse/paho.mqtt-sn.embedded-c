/*******************************************************************************
 * Copyright (c) 2014, 2016 IBM Corp.
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
 *    Ian Craggs - initial API and implementation and/or initial documentation
 *******************************************************************************/

#if !defined(MQTTSNGATEWAY_H)
#define MQTTSNGATEWAY_H

#include "MQTTSNPacket.h"
#include "MQTTConnection.h"

namespace MQTTSN
{
 
#define MAX_PACKET_SIZE 256   
#define MQTTSNGATEWAY_QOS2 0 
#define MQTTSNGATEWAY_QOS1 0
#define MAX_MQTT_CONNECTIONS 5
    
struct Parms
{
  char* hostname;
  int port;
  char* username;
  char* password;  
};

enum QoS { QOS0, QOS1, QOS2 };

enum Modes { AGGREGATING, TRANSPARENT };

// all failure return codes must be negative
enum returnCode { MAX_SUBSCRIPTIONS_EXCEEDED = -3, BUFFER_OVERFLOW = -2, FAILURE = -1, SUCCESS = 0 };

struct Message
{
    enum QoS qos;
    bool retained;
    bool dup;
    unsigned short id;
    void *payload;
    size_t payloadlen;
};


template<class UDPNetwork, class TCPNetwork, class Timer, class Thread, class Mutex>
class Gateway
{
public:

    Gateway(UDPNetwork&, unsigned int command_timeout_ms = 30000);

    void run(void const* arg);
    
private:

    int cycle(Timer& timer);
    int sendPacket(int length, Timer& timer);
    int readPacket(Timer& timer);
    
    UDPNetwork& udpstack;  // Not restricted to UDP -  for want of a better name
    
    unsigned char sendbuf[MAX_PACKET_SIZE];
    unsigned char readbuf[MAX_PACKET_SIZE];
    
    enum Modes mode;
    
    MQTT::Connection<TCPNetwork, Timer> connections[MAX_MQTT_CONNECTIONS];

};


} // end namespace


template<class UDPNetwork, class TCPNetwork, class Timer, class Thread, class Mutex>
MQTTSN::Gateway<UDPNetwork, TCPNetwork, Timer, Thread, Mutex>::Gateway(UDPNetwork& network, unsigned int command_timeout_ms) : udpstack(network)
{
    mode = AGGREGATING;
}


template<class UDPNetwork, class TCPNetwork, class Timer, class Thread, class Mutex>
void MQTTSN::Gateway<UDPNetwork, TCPNetwork, Timer, Thread, Mutex>::run(void const* arg)
{   
    Timer timer;
    
    printf("Gateway run 0\n");
    if (mode == AGGREGATING)
    {
        // set up connection 0 information
        Thread mythread(&connections[0].run); 
    }
    
    printf("Gateway run 1\n");
    while (true)
    {
     		printf("Gateway cycle\n");
        cycle(timer);
    }
    
}


template<class UDPNetwork, class TCPNetwork, class Timer, class Thread, class Mutex>
int MQTTSN::Gateway<UDPNetwork, TCPNetwork, Timer, Thread, Mutex>::cycle(Timer& timer)
{
    /* get one piece of work off the wire and one pass through */

    // read the socket, see what work is due
    unsigned short packet_type = readPacket(timer);
    
    printf("read packet\n");

    int len = 0;
    int rc = SUCCESS;

    switch (packet_type)
    {
        case MQTTSN_CONNECT:
        {
            MQTTSNPacket_connectData data = MQTTSNPacket_connectData_initializer;
            MQTTSNDeserialize_connect(&data, readbuf, MAX_PACKET_SIZE);
            
            if (mode == TRANSPARENT)
            {
                //start a new MQTT connection at this point
            }
            
            len = MQTTSNSerialize_connack(sendbuf, MAX_PACKET_SIZE, 0);
            if (len <= 0)
                rc = FAILURE;
            else
                rc = sendPacket(len, timer);
            break;
        }   
        
        case MQTTSN_REGISTER:
        {
            unsigned short topicid, packetid;
            MQTTSNString topicName;
            unsigned char reg_rc = MQTTSN_RC_ACCEPTED;
            if (MQTTSNDeserialize_register(&topicid, &packetid, &topicName, readbuf, MAX_PACKET_SIZE) != 1)
                goto exit;
                
            // store topic registration info    
                
            len = MQTTSNSerialize_regack(connections[0].sendbuf, MAX_PACKET_SIZE, topicid, packetid, reg_rc);
            if (len <= 0)
                rc = FAILURE;
            else
                rc = connections[0].sendPacket(len, timer);
                
            break;
        }
        
        case MQTTSN_SUBSCRIBE:
        {
            unsigned char dup;
            int qos;
            unsigned short packetid;
            MQTTSN_topicid topicFilter;
            MQTTSNDeserialize_subscribe(&dup, &qos, &packetid, &topicFilter, readbuf, MAX_PACKET_SIZE);
            MQTTString topic = MQTTString_initializer;
            
            if (topicFilter.type == MQTTSN_TOPIC_TYPE_NORMAL)
            {
                topic.lenstring.len = topicFilter.data.long_.len;
                topic.lenstring.data = topicFilter.data.long_.name;
            }
            
            len = MQTTSerialize_subscribe(connections[0].sendbuf, MAX_PACKET_SIZE, 0, packetid, 1, &topic, (int*)&qos);
            if (len <= 0)
                goto exit;
            if ((rc = connections[0].sendPacket(len, timer)) != SUCCESS) // send the subscribe packet
                goto exit;             // there was a problem
            
            break;
        }
        
        case MQTTSN_PUBLISH:
        {
            MQTTString topic = MQTTString_initializer;
            MQTTSN_topicid topicid;
            Message msg;
            
            if (MQTTSNDeserialize_publish((unsigned char*)&msg.dup, (int*)&msg.qos, (unsigned char*)&msg.retained, &msg.id, &topicid,
                                 (unsigned char**)&msg.payload, (int*)&msg.payloadlen, readbuf, MAX_PACKET_SIZE) != 1)
                goto exit;
            
                
            len = MQTTSerialize_publish(connections[0].sendbuf, MAX_PACKET_SIZE, msg.dup, msg.qos, msg.retained, msg.id, topic, 
                                (unsigned char*)msg.payload, msg.payloadlen);
            if (len <= 0)
                goto exit;
            if ((rc = connections[0].sendPacket(len, timer)) != SUCCESS) // send the subscribe packet
                goto exit;             // there was a problem
                
        } 
                
                
        case MQTTSN_PINGRESP:
            //ping_outstanding = false;
            break;
    }
    //keepalive();
exit:
    if (rc == SUCCESS)
        rc = packet_type;
    return rc;
}



template<class UDPNetwork, class TCPNetwork, class Timer, class Thread, class Mutex>
int MQTTSN::Gateway<UDPNetwork, TCPNetwork, Timer, Thread, Mutex>::sendPacket(int length, Timer& timer)
{
    int rc = FAILURE,
        sent = 0;

    do
    {
        sent = udpstack.write(sendbuf, length, timer.left_ms());
        printf("sendPacket, rc %d from write of %d bytes\n", sent, length);
        if (sent < 0)  // there was an error writing the data
            break;
    }
    while (sent != length && !timer.expired());
    
    if (sent == length)
    {
        //if (this->duration > 0)
        //    last_sent.countdown(this->duration); // record the fact that we have successfully sent the packet
        rc = SUCCESS;
    }
    else
        rc = FAILURE;
        
#if defined(MQTT_DEBUG)
    char printbuf[50];
    DEBUG("Rc %d from sending packet %s\n", rc, MQTTSNPacket_toString(printbuf, sizeof(printbuf), sendbuf, length));
#endif
    return rc;
}


/**
 * If any read fails in this method, then we should disconnect from the network, as on reconnect
 * the packets can be retried.
 * @param timeout the max time to wait for the packet read to complete, in milliseconds
 * @return the MQTT packet type, or -1 if none
 */
template<class UDPNetwork, class TCPNetwork, class Timer, class Thread, class Mutex>
int MQTTSN::Gateway<UDPNetwork, TCPNetwork, Timer, Thread, Mutex>::readPacket(Timer& timer)
{
    int rc = FAILURE;
    int len = 0;  // the length of the whole packet including length field 
    int lenlen = 0;
    int datalen = 0;

    #define MQTTSN_MIN_PACKET_LENGTH 3
    // 1. read the packet, datagram style 
    if ((len = udpstack.read(readbuf, MAX_PACKET_SIZE, timer.left_ms())) < MQTTSN_MIN_PACKET_LENGTH)
        goto exit;
        
    // 2. read the length.  This is variable in itself 
    lenlen = MQTTSNPacket_decode(readbuf, len, &datalen);
    if (datalen != len)
        goto exit; // there was an error 
        
    rc = readbuf[lenlen];
    //if (this->duration > 0)
    //    last_received.countdown(this->duration); // record the fact that we have successfully received a packet
exit:
        
#if defined(MQTT_DEBUG)
    char printbuf[50];
    DEBUG("Rc %d from receiving packet %s\n", rc, MQTTSNPacket_toString(printbuf, sizeof(printbuf), readbuf, len));
#endif
    return rc;
}



#endif

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

#if !defined(MQTTCONNECTION_H)
#define MQTTCONNECTION_H

#include "FP.h"
#include "MQTTPacket.h"
#include "stdio.h"
#include "MQTTLogging.h"

#if !defined(MQTTCLIENT_QOS1)
    #define MQTTCLIENT_QOS1 1
#endif
#if !defined(MQTTCLIENT_QOS2)
    #define MQTTCLIENT_QOS2 0
#endif

namespace MQTT
{


enum QoS { QOS0, QOS1, QOS2 };

// all failure return codes must be negative
enum returnCode { BUFFER_OVERFLOW = -2, FAILURE = -1, SUCCESS = 0 };


struct Message
{
    enum QoS qos;
    bool retained;
    bool dup;
    unsigned short id;
    void *payload;
    size_t payloadlen;
};


class PacketId
{
public:
    PacketId()
    {
        next = 0;
    }

    int getNext()
    {
        return next = (next == MAX_PACKET_ID) ? 1 : ++next;
    }

private:
    static const int MAX_PACKET_ID = 65535;
    int next;
};


/**
 * @class Client
 * @brief blocking, non-threaded MQTT client API
 *
 * This version of the API blocks on all method calls, until they are complete.  This means that only one
 * MQTT request can be in process at any one time.
 * @param Network a network class which supports send, receive
 * @param Timer a timer class with the methods:
 */
template<class Network, class Timer, int MAX_MQTT_PACKET_SIZE = 100, int MAX_MESSAGE_HANDLERS = 5>
class Connection
{

public: 

    /** Construct the client
     *  @param network - pointer to an instance of the Network class - must be connected to the endpoint
     *      before calling MQTT connect
     *  @param limits an instance of the Limit class - to alter limits as required
     */
    Connection(unsigned int command_timeout_ms = 30000);
    
    
    static void run(void const* arg);
    

    /** MQTT Disconnect - send an MQTT disconnect packet, and clean up any state
     *  @return success code -
     */
    int disconnect();

    /** Is the client connected?
     *  @return flag - is the client connected or not?
     */
    bool isConnected()
    {
        return isconnected;
    }

    int sendPacket(int length, Timer& timer);
    unsigned char sendbuf[MAX_MQTT_PACKET_SIZE];

private:

    int connect(MQTTPacket_connectData&);
    int connect();
    
    void cleanSession();
    int cycle(Timer& timer);
    int waitfor(int packet_type, Timer& timer);
    int keepalive();
    int publish(int len, Timer& timer, enum QoS qos);

    int decodePacket(int* value, int timeout);
    int readPacket(Timer& timer);

    Network ipstack;
    unsigned long command_timeout_ms;
    unsigned char readbuf[MAX_MQTT_PACKET_SIZE];

    Timer last_sent, last_received;
    unsigned int keepAliveInterval;
    bool ping_outstanding;
    bool cleansession;

    PacketId packetid;

    bool isconnected;

#if MQTTCLIENT_QOS1 || MQTTCLIENT_QOS2
    unsigned char pubbuf[MAX_MQTT_PACKET_SIZE];  // store the last publish for sending on reconnect
    int inflightLen;
    unsigned short inflightMsgid;
    enum QoS inflightQoS;
#endif

#if MQTTCLIENT_QOS2
    bool pubrel;
    #if !defined(MAX_INCOMING_QOS2_MESSAGES)
        #define MAX_INCOMING_QOS2_MESSAGES 10
    #endif
    unsigned short incomingQoS2messages[MAX_INCOMING_QOS2_MESSAGES];
    bool isQoS2msgidFree(unsigned short id);
    bool useQoS2msgid(unsigned short id);
    void freeQoS2msgid(unsigned short id);
#endif

};

}


template<class Network, class Timer, int a, int MAX_MESSAGE_HANDLERS>
void MQTT::Connection<Network, Timer, a, MAX_MESSAGE_HANDLERS>::run(void const* arg)
{
    
    
    
        
}


template<class Network, class Timer, int a, int MAX_MESSAGE_HANDLERS>
void MQTT::Connection<Network, Timer, a, MAX_MESSAGE_HANDLERS>::cleanSession() 
{
    ping_outstanding = false;
    isconnected = false;

#if MQTTCLIENT_QOS1 || MQTTCLIENT_QOS2
    inflightMsgid = 0;
    inflightQoS = QOS0;
#endif

#if MQTTCLIENT_QOS2
    pubrel = false;
    for (int i = 0; i < MAX_INCOMING_QOS2_MESSAGES; ++i)
        incomingQoS2messages[i] = 0;
#endif
}


template<class Network, class Timer, int a, int MAX_MESSAGE_HANDLERS>
MQTT::Connection<Network, Timer, a, MAX_MESSAGE_HANDLERS>::Connection(unsigned int command_timeout_ms) : packetid()
{
    last_sent = Timer();
    last_received = Timer();
    this->command_timeout_ms = command_timeout_ms;
    cleanSession();
}


#if MQTTCLIENT_QOS2
template<class Network, class Timer, int a, int b>
bool MQTT::Connection<Network, Timer, a, b>::isQoS2msgidFree(unsigned short id)
{
    for (int i = 0; i < MAX_INCOMING_QOS2_MESSAGES; ++i)
    {
        if (incomingQoS2messages[i] == id)
            return false;
    }
    return true;
}


template<class Network, class Timer, int a, int b>
bool MQTT::Connection<Network, Timer, a, b>::useQoS2msgid(unsigned short id)
{
    for (int i = 0; i < MAX_INCOMING_QOS2_MESSAGES; ++i)
    {
        if (incomingQoS2messages[i] == 0)
        {
            incomingQoS2messages[i] = id;
            return true;
        }
    }
    return false;
}


template<class Network, class Timer, int a, int b>
void MQTT::Connection<Network, Timer, a, b>::freeQoS2msgid(unsigned short id)
{
    for (int i = 0; i < MAX_INCOMING_QOS2_MESSAGES; ++i)
    {
        if (incomingQoS2messages[i] == id)
        {
            incomingQoS2messages[i] = 0;
            return;
        }
    }
}
#endif


template<class Network, class Timer, int a, int b>
int MQTT::Connection<Network, Timer, a, b>::sendPacket(int length, Timer& timer)
{
    int rc = FAILURE,
        sent = 0;

    while (sent < length && !timer.expired())
    {
        rc = ipstack.write(&sendbuf[sent], length - sent, timer.left_ms());
        if (rc < 0)  // there was an error writing the data
            break;
        sent += rc;
    }
    if (sent == length)
    {
        if (this->keepAliveInterval > 0)
            last_sent.countdown(this->keepAliveInterval); // record the fact that we have successfully sent the packet
        rc = SUCCESS;
    }
    else
        rc = FAILURE;
        
#if defined(MQTT_DEBUG)
    char printbuf[150];
    DEBUG("Rc %d from sending packet %s\n", rc, MQTTFormat_toServerString(printbuf, sizeof(printbuf), sendbuf, length));
#endif
    return rc;
}


template<class Network, class Timer, int a, int b>
int MQTT::Connection<Network, Timer, a, b>::decodePacket(int* value, int timeout)
{
    unsigned char c;
    int multiplier = 1;
    int len = 0;
    const int MAX_NO_OF_REMAINING_LENGTH_BYTES = 4;

    *value = 0;
    do
    {
        int rc = MQTTPACKET_READ_ERROR;

        if (++len > MAX_NO_OF_REMAINING_LENGTH_BYTES)
        {
            rc = MQTTPACKET_READ_ERROR; /* bad data */
            goto exit;
        }
        rc = ipstack.read(&c, 1, timeout);
        if (rc != 1)
            goto exit;
        *value += (c & 127) * multiplier;
        multiplier *= 128;
    } while ((c & 128) != 0);
exit:
    return len;
}


/**
 * If any read fails in this method, then we should disconnect from the network, as on reconnect
 * the packets can be retried.
 * @param timeout the max time to wait for the packet read to complete, in milliseconds
 * @return the MQTT packet type, or -1 if none
 */
template<class Network, class Timer, int MAX_MQTT_PACKET_SIZE, int b>
int MQTT::Connection<Network, Timer, MAX_MQTT_PACKET_SIZE, b>::readPacket(Timer& timer)
{
    int rc = FAILURE;
    MQTTHeader header = {0};
    int len = 0;
    int rem_len = 0;

    /* 1. read the header byte.  This has the packet type in it */
    if (ipstack.read(readbuf, 1, timer.left_ms()) != 1)
        goto exit;

    len = 1;
    /* 2. read the remaining length.  This is variable in itself */
    decodePacket(&rem_len, timer.left_ms());
    len += MQTTPacket_encode(readbuf + 1, rem_len); /* put the original remaining length into the buffer */

    if (rem_len > (MAX_MQTT_PACKET_SIZE - len))
    {
        rc = BUFFER_OVERFLOW;
        goto exit;
    }

    /* 3. read the rest of the buffer using a callback to supply the rest of the data */
    if (rem_len > 0 && (ipstack.read(readbuf + len, rem_len, timer.left_ms()) != rem_len))
        goto exit;

    header.byte = readbuf[0];
    rc = header.bits.type;
    if (this->keepAliveInterval > 0)
        last_received.countdown(this->keepAliveInterval); // record the fact that we have successfully received a packet
exit:
        
#if defined(MQTT_DEBUG)
    if (rc >= 0)
    {
        char printbuf[50];
        DEBUG("Rc %d from receiving packet %s\n", rc, MQTTFormat_toClientString(printbuf, sizeof(printbuf), readbuf, len));
    }
#endif
    return rc;
}


template<class Network, class Timer, int MAX_MQTT_PACKET_SIZE, int b>
int MQTT::Connection<Network, Timer, MAX_MQTT_PACKET_SIZE, b>::cycle(Timer& timer)
{
    /* get one piece of work off the wire and one pass through */

    // read the socket, see what work is due
    int packet_type = readPacket(timer);

    int len = 0,
        rc = SUCCESS;

    switch (packet_type)
    {
        case FAILURE:
        case BUFFER_OVERFLOW:
            rc = packet_type;
            break;
        case CONNACK:
        case PUBACK:
        case SUBACK:
            break;
        case PUBLISH:
        {
            MQTTString topicName = MQTTString_initializer;
            Message msg;
            int intQoS;
            if (MQTTDeserialize_publish((unsigned char*)&msg.dup, &intQoS, (unsigned char*)&msg.retained, (unsigned short*)&msg.id, &topicName,
                                 (unsigned char**)&msg.payload, (int*)&msg.payloadlen, readbuf, MAX_MQTT_PACKET_SIZE) != 1)
                goto exit;
            msg.qos = (enum QoS)intQoS;
#if MQTTCLIENT_QOS2
            if (msg.qos != QOS2)
#endif
                ; //deliverMessage(topicName, msg);
#if MQTTCLIENT_QOS2
            else if (isQoS2msgidFree(msg.id))
            {
                if (useQoS2msgid(msg.id))
                    ; //deliverMessage(topicName, msg);
                else
                    WARN("Maximum number of incoming QoS2 messages exceeded");
            }
#endif
#if MQTTCLIENT_QOS1 || MQTTCLIENT_QOS2
            if (msg.qos != QOS0)
            {
                if (msg.qos == QOS1)
                    len = MQTTSerialize_ack(sendbuf, MAX_MQTT_PACKET_SIZE, PUBACK, 0, msg.id);
                else if (msg.qos == QOS2)
                    len = MQTTSerialize_ack(sendbuf, MAX_MQTT_PACKET_SIZE, PUBREC, 0, msg.id);
                if (len <= 0)
                    rc = FAILURE;
                else
                    rc = sendPacket(len, timer);
                if (rc == FAILURE)
                    goto exit; // there was a problem
            }
            break;
#endif
        }
#if MQTTCLIENT_QOS2
        case PUBREC:
        case PUBREL:
            unsigned short mypacketid;
            unsigned char dup, type;
            if (MQTTDeserialize_ack(&type, &dup, &mypacketid, readbuf, MAX_MQTT_PACKET_SIZE) != 1)
                rc = FAILURE;
            else if ((len = MQTTSerialize_ack(sendbuf, MAX_MQTT_PACKET_SIZE, 
                        (packet_type == PUBREC) ? PUBREL : PUBCOMP, 0, mypacketid)) <= 0)
                rc = FAILURE;
            else if ((rc = sendPacket(len, timer)) != SUCCESS) // send the PUBREL packet
                rc = FAILURE; // there was a problem
            if (rc == FAILURE)
                goto exit; // there was a problem
            if (packet_type == PUBREL)
                freeQoS2msgid(mypacketid);
            break;
            
        case PUBCOMP:
            break;
#endif
        case PINGRESP:
            ping_outstanding = false;
            break;
    }
    keepalive();
exit:
    if (rc == SUCCESS)
        rc = packet_type;
    return rc;
}


template<class Network, class Timer, int MAX_MQTT_PACKET_SIZE, int b>
int MQTT::Connection<Network, Timer, MAX_MQTT_PACKET_SIZE, b>::keepalive()
{
    int rc = FAILURE;

    if (keepAliveInterval == 0)
    {
        rc = SUCCESS;
        goto exit;
    }

    if (last_sent.expired() || last_received.expired())
    {
        if (!ping_outstanding)
        {
            Timer timer(1000);
            int len = MQTTSerialize_pingreq(sendbuf, MAX_MQTT_PACKET_SIZE);
            if (len > 0 && (rc = sendPacket(len, timer)) == SUCCESS) // send the ping packet
                ping_outstanding = true;
        }
    }

exit:
    return rc;
}


template<class Network, class Timer, int MAX_MQTT_PACKET_SIZE, int b>
int MQTT::Connection<Network, Timer, MAX_MQTT_PACKET_SIZE, b>::connect(MQTTPacket_connectData& options)
{
    Timer connect_timer(command_timeout_ms);
    int rc = FAILURE;
    int len = 0;

    if (isconnected) // don't send connect packet again if we are already connected
        goto exit;

    this->keepAliveInterval = options.keepAliveInterval;
    this->cleansession = options.cleansession;
    if ((len = MQTTSerialize_connect(sendbuf, MAX_MQTT_PACKET_SIZE, &options)) <= 0)
        goto exit;
    if ((rc = sendPacket(len, connect_timer)) != SUCCESS)  // send the connect packet
        goto exit; // there was a problem

    if (this->keepAliveInterval > 0)
        last_received.countdown(this->keepAliveInterval);
    // this will be a blocking call, wait for the connack
    if (waitfor(CONNACK, connect_timer) == CONNACK)
    {
        unsigned char connack_rc = 255;
        bool sessionPresent = false;
        if (MQTTDeserialize_connack((unsigned char*)&sessionPresent, &connack_rc, readbuf, MAX_MQTT_PACKET_SIZE) == 1)
            rc = connack_rc;
        else
            rc = FAILURE;
    }
    else
        rc = FAILURE;

#if MQTTCLIENT_QOS2
    // resend any inflight publish
    if (inflightMsgid > 0 && inflightQoS == QOS2 && pubrel)
    {
        if ((len = MQTTSerialize_ack(sendbuf, MAX_MQTT_PACKET_SIZE, PUBREL, 0, inflightMsgid)) <= 0)
            rc = FAILURE;
        else
            rc = publish(len, connect_timer, inflightQoS);
    }
    else
#endif
#if MQTTCLIENT_QOS1 || MQTTCLIENT_QOS2
    if (inflightMsgid > 0)
    {
        memcpy(sendbuf, pubbuf, MAX_MQTT_PACKET_SIZE);
        rc = publish(inflightLen, connect_timer, inflightQoS);
    }
#endif

exit:
    if (rc == SUCCESS)
        isconnected = true;
    return rc;
}


template<class Network, class Timer, int MAX_MQTT_PACKET_SIZE, int b>
int MQTT::Connection<Network, Timer, MAX_MQTT_PACKET_SIZE, b>::connect()
{
    MQTTPacket_connectData default_options = MQTTPacket_connectData_initializer;
    return connect(default_options);
}



template<class Network, class Timer, int MAX_MQTT_PACKET_SIZE, int b>
int MQTT::Connection<Network, Timer, MAX_MQTT_PACKET_SIZE, b>::disconnect()
{
    int rc = FAILURE;
    Timer timer(command_timeout_ms);            // we might wait for incomplete incoming publishes to complete
    int len = MQTTSerialize_disconnect(sendbuf, MAX_MQTT_PACKET_SIZE);
    if (len > 0)
        rc = sendPacket(len, timer);            // send the disconnect packet

    if (cleansession)
        cleanSession();
    else
        isconnected = false;
    return rc;
}


#endif

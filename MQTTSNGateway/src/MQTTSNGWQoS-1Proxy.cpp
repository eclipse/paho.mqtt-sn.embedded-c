/**************************************************************************************
 * Copyright (c) 2018, Tomoaki Yamaguchi
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


#include "MQTTSNGWQoS-1Proxy.h"

#include "MQTTSNGWDefines.h"
#include "MQTTSNGateway.h"
#include "SensorNetwork.h"
#include <string>
#include <string.h>
#include <stdio.h>

using namespace MQTTSNGW;

#define KEEPALIVE_DURATION    900       // Secs
#define RESPONSE_DURATION     10       // Secs

/*
 *     Class ClientProxyElement
 */

QoSm1ProxyElement::QoSm1ProxyElement(void)
    : _clientId{0}
    , _next{0}
{

}

QoSm1ProxyElement::QoSm1ProxyElement(SensorNetAddress* addr,  string* clientId)
    : _next{0}
{
    _clientId = *clientId;
    _sensorNetAddr = *addr;
}

QoSm1ProxyElement::~QoSm1ProxyElement(void)
{

}

/*
 *     Class ClientProxy
 */

QoSm1Proxy:: QoSm1Proxy(void)
    : _head{0}
{
    _gateway = 0;
    _client = 0;
}

QoSm1Proxy:: QoSm1Proxy(Gateway* gw)
    : _head{0}
{
    _gateway = gw;
    _client = 0;
}


QoSm1Proxy::~QoSm1Proxy(void)
{
    if ( _head )
    {
        QoSm1ProxyElement* p = _head;
        while ( p )
        {
            QoSm1ProxyElement* next = p->_next;
            delete p;
            p = next;
        }
    }
}

void QoSm1Proxy::setGateway(Gateway* gw)
{
    _gateway = gw;
}

QoSm1ProxyElement* QoSm1Proxy::add(SensorNetAddress* addr,  string* clientId)
{
    QoSm1ProxyElement* elm = new QoSm1ProxyElement(addr, clientId);
    if ( _head == 0 )
    {
        _head = elm;
    }
    else
    {
        QoSm1ProxyElement* p = _head;
        while ( p )
        {
            if ( p->_next == 0 )
            {
                p->_next = elm;
                break;
            }
            else
            {
                p = p->_next;
            }
        }
    }
    return elm;
}

const char*  QoSm1Proxy::getClientId(SensorNetAddress* addr)
{
    QoSm1ProxyElement* p = _head;
    while ( p )
    {
        if ( p->_sensorNetAddr.isMatch(addr) )
        {
            return p->_clientId.c_str();
            break;
        }
        p = p->_next;
    }
    return 0;
}

void QoSm1Proxy::setClient(Client* client)
{
    _client = client;
}

Client* QoSm1Proxy::getClient(void)
{
    return _client;
}

bool QoSm1Proxy::setClientProxy(const char* fileName)
{
    FILE* fp;
    char buf[MAX_CLIENTID_LENGTH + 256];
    size_t pos;

    SensorNetAddress netAddr;

    if ((fp = fopen(fileName, "r")) != 0)
    {
        while (fgets(buf, MAX_CLIENTID_LENGTH + 254, fp) != 0)
        {
            if (*buf == '#')
            {
                continue;
            }
            string data = string(buf);
            while ((pos = data.find_first_of(" 　\t\n")) != string::npos)
            {
                data.erase(pos, 1);
            }
            if (data.empty())
            {
                continue;
            }

            pos = data.find_first_of(",");
            string id = data.substr(0, pos);
            string addr = data.substr(pos + 1);

            if (netAddr.setAddress(&addr) == 0)
            {
                add(&netAddr, &id);
            }
            else
            {
                WRITELOG("Invalid address     %s\n", data.c_str());
                return false;
            }
        }
        fclose(fp);
    }
    else
    {
        WRITELOG("Can not open the QoS_1Client  List.     %s\n", fileName);
        return false;
    }
    return true;
}


void QoSm1Proxy::checkConnection(void)
{
    if ( _client->isDisconnect()  || ( _client->isConnecting() && _responseTimer.isTimeup()) )
    {
        _client->connectSended();
        _responseTimer.start(RESPONSE_DURATION * 1000UL);
        MQTTSNPacket_connectData options = MQTTSNPacket_connectData_initializer;
        options.clientID.cstring = _client->getClientId();
        options.duration = KEEPALIVE_DURATION;

        MQTTSNPacket* packet = new MQTTSNPacket();
        packet->setCONNECT(&options);
        Event* ev = new Event();
        ev->setClientRecvEvent(_client, packet);
        _gateway->getPacketEventQue()->post(ev);

    }
    else if ( _client->isActive() && _keepAliveTimer.isTimeup() )
    {
            MQTTSNPacket* packet = new MQTTSNPacket();
            MQTTSNString clientId = MQTTSNString_initializer;
            packet->setPINGREQ(&clientId);
            Event* ev = new Event();
            ev->setClientRecvEvent(_client, packet);
            _gateway->getPacketEventQue()->post(ev);
            resetPingTimer();
    }
}

void QoSm1Proxy::resetPingTimer(void)
{
    _keepAliveTimer.start(KEEPALIVE_DURATION * 1000UL);
}

void QoSm1Proxy::send(MQTTSNPacket* packet)
{
    if ( packet->getType() == MQTTSN_CONNACK )
    {
        resetPingTimer();
        sendStoredPublish();
    }
    else if ( packet->getType() == MQTTSN_PINGRESP )
    {
        resetPingTimer();
    }
    else if ( packet->getType() == MQTTSN_DISCONNECT )
    {
        // blank
    }
}

void QoSm1Proxy::sendStoredPublish(void)
{
    MQTTSNPacket* msg = 0;

    while  ( ( msg = _client->getProxyPacket() ) != 0 )
    {
        _client->deleteFirstProxyPacket();  // pop the que to delete element.

        Event* ev = new Event();
        ev->setClientRecvEvent(_client, msg);
        _gateway->getPacketEventQue()->post(ev);
    }
}

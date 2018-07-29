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


#include "MQTTSNGWDefines.h"
#include "MQTTSNGWClientProxy.h"
#include "MQTTSNGateway.h"
#include "SensorNetwork.h"
#include <string>
#include <string.h>
#include <stdio.h>

using namespace MQTTSNGW;

#define RESPONSE_DURATION    900       // Secs

/*
 *     Class ClientProxyElement
 */

ClientProxyElement::ClientProxyElement(void)
    : _clientId{0}
    , _next{0}
{

}

ClientProxyElement::ClientProxyElement(SensorNetAddress* addr,  string* clientId)
    : _next{0}
{
    _clientId = *clientId;
    _sensorNetAddr = *addr;
}

ClientProxyElement::~ClientProxyElement(void)
{

}

/*
 *     Class ClientProxy
 */

ClientProxy:: ClientProxy(void)
    : _head{0}
{
    _gateway = 0;
    _client = 0;
}

ClientProxy:: ClientProxy(Gateway* gw)
    : _head{0}
{
    _gateway = gw;
    _client = 0;
}


ClientProxy::~ClientProxy(void)
{
    if ( _head )
    {
        ClientProxyElement* p = _head;
        while ( p )
        {
            ClientProxyElement* next = p->_next;
            delete p;
            p = next;
        }
    }
}

void ClientProxy::setGateway(Gateway* gw)
{
    _gateway = gw;
}

ClientProxyElement* ClientProxy::add(SensorNetAddress* addr,  string* clientId)
{
    ClientProxyElement* elm = new ClientProxyElement(addr, clientId);
    if ( _head == 0 )
    {
        _head = elm;
    }
    else
    {
        ClientProxyElement* p = _head;
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

const char*  ClientProxy::getClientId(SensorNetAddress* addr)
{
    ClientProxyElement* p = _head;
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

void ClientProxy::setClient(Client* client)
{
    _client = client;
}

Client* ClientProxy::getClient(void)
{
    return _client;
}

bool ClientProxy::setClientProxy(const char* fileName)
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


void ClientProxy::checkConnection(void)
{
    if ( _client->isDisconnect()  || ( _client->isConnecting() && _responseTimer.isTimeup()) )
    {
        _client->connectSended();
        _responseTimer.start(RESPONSE_DURATION * 1000UL);
        MQTTSNPacket_connectData options = MQTTSNPacket_connectData_initializer;
        options.clientID.cstring = _client->getClientId();
        options.duration = RESPONSE_DURATION;

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

void ClientProxy::resetPingTimer(void)
{
    _keepAliveTimer.start(RESPONSE_DURATION * 1000UL);
}

void ClientProxy::send(MQTTSNPacket* packet)
{
    if ( packet->getType() == MQTTSN_CONNACK || packet->getType() == MQTTSN_PINGRESP )
    {
        resetPingTimer();
        sendStoredPublish();
    }
    else if ( packet->getType() == MQTTSN_PINGRESP )
    {
        resetPingTimer();
    }
}

void ClientProxy::sendStoredPublish(void)
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

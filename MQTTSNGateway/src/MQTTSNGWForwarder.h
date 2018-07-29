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

#ifndef MQTTSNGATEWAY_SRC_MQTTSNGWFORWARDER_H_
#define MQTTSNGATEWAY_SRC_MQTTSNGWFORWARDER_H_

#include "MQTTSNGWClient.h"
#include "MQTTSNGWEncapsulatedPacket.h"
#include "SensorNetwork.h"


namespace MQTTSNGW
{

class Client;
class WirelessNodeId;

class ForwarderElement
{
    friend class Forwarder;
public:
    ForwarderElement();
    ~ForwarderElement();
    void setClient(Client* client);
    void setWirelessNodeId(WirelessNodeId* id);
private:
    Client* _client;
    WirelessNodeId* _wirelessNodeId;
    ForwarderElement* _next;
};


class Forwarder
{
    friend class ForwarderList;
public:
    Forwarder();
    Forwarder(SensorNetAddress* addr,  string* forwarderId);
    ~Forwarder();

    const char* getId(void);
    void addClient(Client* client, WirelessNodeId* id);
    Client* getClient(WirelessNodeId* id);
    WirelessNodeId* getWirelessNodeId(Client* client);
    void eraseClient(Client* client);
    SensorNetAddress* getSensorNetAddr(void);
    const char* getName(void);

private:
    string _forwarderName;
    SensorNetAddress _sensorNetAddr;
    ForwarderElement* _headClient;
    Forwarder* _next;
    Mutex _mutex;
};

class ForwarderList
{
public:
    ForwarderList();
    ~ForwarderList();

    Forwarder* getForwarder(SensorNetAddress* addr);
    bool setFowerder(const char* fileName);
    Forwarder* addForwarder(SensorNetAddress* addr,  string* forwarderId);

private:
    Forwarder* _head;
};

}



#endif /* MQTTSNGATEWAY_SRC_MQTTSNGWFORWARDER_H_ */

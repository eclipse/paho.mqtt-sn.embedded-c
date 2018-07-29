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

#ifndef MQTTSNGATEWAY_SRC_MQTTSNGWCLIENTPROXY_H_
#define MQTTSNGATEWAY_SRC_MQTTSNGWCLIENTPROXY_H_

#include "MQTTSNGateway.h"
#include "MQTTGWPacket.h"
#include "MQTTSNGWClient.h"
#include "SensorNetwork.h"
#include "MQTTSNGWProcess.h"



namespace MQTTSNGW
{
class Gateway;

class ClientProxyElement
{
    friend class ClientProxy;
public:
    ClientProxyElement(void);
    ClientProxyElement(SensorNetAddress* addr,  string* clientId);
    ~ClientProxyElement(void);
private:
    SensorNetAddress  _sensorNetAddr;
    string  _clientId;
    ClientProxyElement* _next;
};

class ClientProxy
{
public:
    ClientProxy(void);
    ClientProxy(Gateway* gw);
    ~ClientProxy(void);
    bool setClientProxy(const char* fileName);
    ClientProxyElement* add(SensorNetAddress* addr,  string* clientId);
    const char* getClientId(SensorNetAddress* addr);
    void setClient(Client*);
    Client* getClient(void);
    void setGateway(Gateway* gw);
    void setKeepAlive(uint16_t secs);

    void checkConnection(void);
    void resetPingTimer(void);
    void send(MQTTSNPacket* packet);

private:
    void sendStoredPublish(void);

    Gateway* _gateway;
    Client* _client;
    ClientProxyElement* _head;
    Timer  _keepAliveTimer;
    Timer  _responseTimer;
};

}



#endif /* MQTTSNGATEWAY_SRC_MQTTSNGWCLIENTPROXY_H_ */

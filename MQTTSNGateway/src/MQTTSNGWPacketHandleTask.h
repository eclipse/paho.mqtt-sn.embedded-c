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
 *    Tomoaki Yamaguchi - initial API and implementation and/or initial documentation
 **************************************************************************************/

#ifndef MQTTSNGWPACKETHANDLETASK_H_
#define MQTTSNGWPACKETHANDLETASK_H_

#include "MQTTSNGWDefines.h"
#include "MQTTSNGateway.h"
#include "MQTTSNGWClient.h"
#include "MQTTSNGWPacket.h"
#include "MQTTGWPacket.h"
#include "MQTTGWConnectionHandler.h"
#include "MQTTGWPublishHandler.h"
#include "MQTTGWSubscribeHandler.h"
#include "MQTTSNGWConnectionHandler.h"
#include "MQTTSNGWPublishHandler.h"
#include "MQTTSNGWSubscribeHandler.h"
#include "SensorNetwork.h"
#include "linux.cpp"


namespace MQTTSNGW
{

#define ERRNO_APL_01  11    // Task Initialize Error

/*=====================================
        Class PacketHandleTask
 =====================================*/
class PacketHandleTask : public Thread
{
	MAGIC_WORD_FOR_THREAD;
public:
	PacketHandleTask(Gateway* gateway);
	~PacketHandleTask();
	void run();
private:
	Gateway* _gateway;
	Timer _advertiseTimer;
	Timer _sendUnixTimer;
	MQTTGWConnectionHandler* _mqttConnection;
	MQTTGWPublishHandler*    _mqttPublish;
	MQTTGWSubscribeHandler*  _mqttSubscribe;
	MQTTSNConnectionHandler* _mqttsnConnection;
	MQTTSNPublishHandler*    _mqttsnPublish;
	MQTTSNSubscribeHandler*  _mqttsnSubscribe;
};


}

#endif /* MQTTSNGWPACKETHANDLETASK_H_ */

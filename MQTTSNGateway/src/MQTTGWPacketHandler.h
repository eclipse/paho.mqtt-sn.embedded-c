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
#ifndef MQTTGWPACKETHANDLER_H_
#define MQTTGWPACKETHANDLER_H_

namespace MQTTSNGW
{
#include "MQTTSNGWDefines.h"
#include "MQTTSNPacket.h"
#include "MQTTSNGWClient.h"

class MQTTGWPacketHandler
{
public:
	MQTTGWPacketHandler();
	MQTTGWPacketHandler(Gateway* gateway);
	~MQTTGWPacketHandler();
	int handle(Client* client, MQTTGWPacket* packet);

private:
	void handlePuback(Client* clnode, MQTTGWPacket* packet);
	void handlePingresp(Client* clnode, MQTTGWPacket* packet);
	void handleSuback(Client* clnode, MQTTGWPacket* packet);
	void handleUnsuback(Client* clnode, MQTTGWPacket* packet);
	void handleConnack(Client* clnode, MQTTGWPacket* packet);
	void handlePublish(Client* clnode, MQTTGWPacket* packet);
	void handleDisconnect(Client* clnode, MQTTGWPacket* packet);
	void handlePubRec(Client* clnode, MQTTGWPacket* packet);
	void handlePubRel(Client* clnode, MQTTGWPacket* packet);
	void handlePubComp(Client* clnode, MQTTGWPacket* packet);

	Gateway* _gateway;
};

}

#endif /* MQTTGWPACKETHANDLER_H_ */

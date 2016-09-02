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
#include "MQTTSNGateway.h"
#include "MQTTSNGWBrokerRecvTask.h"
#include "MQTTSNGWBrokerSendTask.h"
#include "MQTTSNGWClientRecvTask.h"
#include "MQTTSNGWClientSendTask.h"
#include "MQTTSNGWPacketHandleTask.h"

using namespace MQTTSNGW;

/*
 *  Gateway Process
 */
Gateway* gateway = new Gateway();
PacketHandleTask* t0 = new PacketHandleTask(gateway);
ClientRecvTask* t1 = new ClientRecvTask(gateway);
ClientSendTask* t2 = new ClientSendTask(gateway);
BrokerRecvTask* t3 = new BrokerRecvTask(gateway);
BrokerSendTask* t4 = new BrokerSendTask(gateway);

int main(int argc, char** argv)
{
	gateway->initialize(argc, argv);
	gateway->run();
	delete gateway;
	return 0;
}


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
#ifndef MQTTSNGWCLIENTRECVTASK_H_
#define MQTTSNGWCLIENTRECVTASK_H_

#include "SensorNetwork.h"
#include "MQTTSNGateway.h"

namespace MQTTSNGW
{

/*=====================================
     Class ClientRecvTask
 =====================================*/
class ClientRecvTask:public Thread
{
	MAGIC_WORD_FOR_THREAD;
public:
	ClientRecvTask(Gateway*);
	~ClientRecvTask();
	virtual void initialize(int argc, char** argv);
	void run();

private:
	void log(Client*, MQTTSNPacket*, MQTTSNString* id = 0);

	Gateway*       _gateway;
	SensorNetwork* _sensorNetwork;
};

}

#endif /* MQTTSNGWCLIENTRECVTASK_H_ */

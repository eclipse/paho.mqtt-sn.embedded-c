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

#include "MQTTSNGWBrokerSendTask.h"
#include "MQTTSNGWDefines.h"
#include "MQTTSNGateway.h"
#include "MQTTSNGWClient.h"
#include "MQTTGWPacket.h"
#include <string.h>

using namespace std;
using namespace MQTTSNGW;

char* currentDateTime();
#define ERRMSG_FORMAT "\n%s   \x1b[0m\x1b[31merror:\x1b[0m\x1b[37m Can't Xmit to the Broker. errno=%d\n"

/*=====================================
 Class BrokerSendTask
 =====================================*/
BrokerSendTask::BrokerSendTask(Gateway* gateway)
{
	_gateway = gateway;
	_gateway->attach((Thread*)this);
	_gwparams = 0;
	_light = 0;
}

BrokerSendTask::~BrokerSendTask()
{

}

/**
 *  Initialize attributs of this class
 */
void BrokerSendTask::initialize(int argc, char** argv)
{
	_gwparams = _gateway->getGWParams();
	_light = _gateway->getLightIndicator();
}

/**
 *  connect to the broker and send MQTT messges
 */
void BrokerSendTask::run()
{
	Event* ev = 0;
	MQTTGWPacket* packet = 0;
	Client* client = 0;

	while (true)
	{
		int rc = 0;
		ev = _gateway->getBrokerSendQue()->wait();
		client = ev->getClient();
		packet = ev->getMQTTGWPacket();
		/*
		if ( client->getNetwork()->isValid() && !client->getNetwork()->isSecure() && packet->getType() == CONNECT )
		{
			client->getNetwork()->close();
		}
		*/
		if ( !client->getNetwork()->isValid() )
		{
			/* connect to the broker and send a packet */
			char* portNo = _gwparams->port;
			const char* cert = 0;
			const char* keyFile = 0;
			string certFile;
			string privateKeyFile;

			if (client->isSecureNetwork())
			{
				portNo = _gwparams->portSecure;
				if ( _gwparams->certDirectory )
				{
					certFile = _gwparams->certDirectory;
					certFile += client->getClientId();
					certFile += ".crt";
					cert = certFile.c_str();
					privateKeyFile = _gwparams->certDirectory;
					privateKeyFile += client->getClientId();
					privateKeyFile += ".key";
					keyFile = privateKeyFile.c_str();
				}
				rc = client->getNetwork()->connect(_gwparams->brokerName, _gwparams->portSecure, _gwparams->rootCApath, _gwparams->rootCAfile, cert, keyFile);
			}
			else
			{
				rc = client->getNetwork()->connect(_gwparams->brokerName, portNo);
			}

			if ( !rc )
			{
				/* disconnect the broker and the client */
				WRITELOG("%s BrokerSendTask can't connect to the broker.  errno=%d %s%s\n",
						ERRMSG_HEADER, rc == -1 ? errno : 0, client->getClientId(), ERRMSG_FOOTER);
				delete ev;
				disconnect(client);
				client->getNetwork()->disconnect();
				continue;
			}
		}

		/* send a packet */
		_light->blueLight(true);
		if ( (rc = packet->send(client->getNetwork())) > 0 )
		{
			if ( packet->getType() == CONNECT )
			{
				client->setWaitWillMsgFlg(false);
				client->connectSended();
			}
			log(client, packet);
		}
		else
		{
			WRITELOG("%s BrokerSendTask can't send a packet.  errno=%d %s%s\n",
				ERRMSG_HEADER, rc == -1 ? errno : 0, client->getClientId(), ERRMSG_FOOTER);
				WRITELOG("%s BrokerSendTask can't send a packet to the broker errno=%d %s%s\n",
						ERRMSG_HEADER, rc == -1 ? errno : 0, client->getClientId(), ERRMSG_FOOTER);
				client->disconnected();
				client->getNetwork()->disconnect();
		}

		_light->blueLight(false);
		delete ev;
	}
}


/**
 *  write message content into stdout or Ringbuffer
 */
void BrokerSendTask::log(Client* client, MQTTGWPacket* packet)
{
	char pbuf[(SIZE_OF_LOG_PACKET + 5 )* 3];
	char msgId[6];

	switch (packet->getType())
	{
	case CONNECT:
		WRITELOG(FORMAT_Y_Y_W, currentDateTime(), packet->getName(), RIGHTARROW, client->getClientId(), packet->print(pbuf));
		break;
	case PUBLISH:
		WRITELOG(FORMAT_W_MSGID_Y_W, currentDateTime(), packet->getName(), packet->getMsgId(msgId), RIGHTARROW, client->getClientId(), packet->print(pbuf));
		break;
	case SUBSCRIBE:
	case UNSUBSCRIBE:
	case PUBACK:
	case PUBREC:
	case PUBREL:
	case PUBCOMP:
		WRITELOG(FORMAT_W_MSGID_Y_W, currentDateTime(), packet->getName(), packet->getMsgId(msgId), RIGHTARROW, client->getClientId(), packet->print(pbuf));
		break;
	case PINGREQ:
		WRITELOG(FORMAT_Y_Y_W, currentDateTime(), packet->getName(), RIGHTARROW, client->getClientId(), packet->print(pbuf));
		break;
	case DISCONNECT:
		WRITELOG(FORMAT_Y_Y_W, currentDateTime(), packet->getName(), RIGHTARROW, client->getClientId(), packet->print(pbuf));
		break;
	default:
		break;
	}
}

void BrokerSendTask::disconnect(Client* client)
{
	MQTTSNPacket* snMsg = new MQTTSNPacket();
	snMsg->setDISCONNECT(0);
	Event* ev1 = new Event();
	ev1->setClientSendEvent(client, snMsg);
	_gateway->getClientSendQue()->post(ev1);
	client->disconnected();
}

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
	_host = 0;
	_service = 0;
	_light = 0;
}

BrokerSendTask::~BrokerSendTask()
{
	if (_host)
	{
		free(_host);
	}
	if (_service)
	{
		free(_service);
	}
}

/**
 *  Initialize attributs of this class
 */
void BrokerSendTask::initialize(int argc, char** argv)
{
	char param[MQTTSNGW_PARAM_MAX];

	if (_gateway->getParam("BrokerName", param) == 0)
	{
		_host = strdup(param);
	}
	if (_gateway->getParam("BrokerPortNo", param) == 0)
	{
		_service = strdup(param);
	}
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

		if ( client->getNetwork()->isValid() && packet->getType() == CONNECT )
		{
			client->getNetwork()->close();
		}

		if ( !client->getNetwork()->isValid() )
		{
			/* connect to the broker and send a packet */
			if ( !client->getNetwork()->connect(_host, _service) )
			{
				/* disconnect the broker and chage the client's status */
				WRITELOG("%s BrokerSendTask can't open the socket.  errno=%d %s%s\n",
						ERRMSG_HEADER, rc == -1 ? errno : 0, client->getClientId(), ERRMSG_FOOTER);
				client->disconnected();
				client->getNetwork()->disconnect();
				delete ev;
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
	char pbuf[(SIZEOF_LOG_PACKET + 5 )* 3];
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

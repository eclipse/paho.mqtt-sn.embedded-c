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

#include "MQTTSNGWBrokerRecvTask.h"
#include "MQTTSNGWClient.h"

using namespace std;
using namespace MQTTSNGW;

char* currentDateTime(void);

/*=====================================
 Class BrokerRecvTask
 =====================================*/
BrokerRecvTask::BrokerRecvTask(Gateway* gateway)
{
	_gateway = gateway;
	_gateway->attach((Thread*)this);
	_light = 0;
}

BrokerRecvTask::~BrokerRecvTask()
{

}

/**
 *  Initialize attributs of this class
 */
void BrokerRecvTask::initialize(int argc, char** argv)
{
	_light = _gateway->getLightIndicator();
}

/**
 *  receive a MQTT messge from the broker and post a event.
 */
void BrokerRecvTask::run(void)
{
	struct timeval timeout;
	MQTTGWPacket* packet = 0;
	int rc;
	Event* ev = 0;
	fd_set rset;
	fd_set wset;

	while (true)
	{
		if (CHK_SIGINT)
		{
			return;
		}
		timeout.tv_sec = 0;
		timeout.tv_usec = 500000;    // 500 msec
		FD_ZERO(&rset);
		FD_ZERO(&wset);
		int maxSock = 0;
		int sockfd = 0;

		/* Prepare sockets list to read */
		Client* client = _gateway->getClientList()->getClient();
		_light->blueLight(false);

		while (client > 0)
		{
			if (client->getNetwork()->isValid())
			{
				sockfd = client->getNetwork()->getSock();
				FD_SET(sockfd, &rset);
				FD_SET(sockfd, &wset);
				if (sockfd > maxSock)
				{
					maxSock = sockfd;
				}
			}
			client = client->getNextClient();
		}

		if (maxSock > 0)
		{
			/* Check sockets is ready to read */
			int activity = select(maxSock + 1, &rset, 0, 0, &timeout);
			if (activity > 0)
			{
				client = _gateway->getClientList()->getClient();

				while (client > 0)
				{
					_light->blueLight(false);
					if (client->getNetwork()->isValid())
					{
						int sockfd = client->getNetwork()->getSock();
						if (FD_ISSET(sockfd, &rset))
						{
							packet = new MQTTGWPacket();
							rc = 0;
							/* read sockets */
							_light->blueLight(true);
							rc = packet->recv(client->getNetwork());
							if ( rc > 0 )
							{
								if ( log(client, packet) == -1 )

								{
									continue;
								}

								/* post a BrokerRecvEvent */
								ev = new Event();
								ev->setBrokerRecvEvent(client, packet);
								_gateway->getPacketEventQue()->post(ev);
							}
							else
							{
								_light->blueLight(false);
								if ( rc == 0 )
								{
									delete packet;
									continue;
								}
								else if (rc == -1)
								{
									WRITELOG("%s BrokerRecvTask can't receive a packet from the broker errno=%d %s%s\n", ERRMSG_HEADER, errno, client->getClientId(), ERRMSG_FOOTER);
								}
								else if ( rc == -2 )
								{
									WRITELOG("%s BrokerRecvTask receive invalid length of packet from the broker.  DISCONNECT  %s %s\n", ERRMSG_HEADER, client->getClientId(),ERRMSG_FOOTER);
								}
								else if ( rc == -3 )
								{
									WRITELOG("%s BrokerRecvTask can't create the packet %s%s\n", ERRMSG_HEADER, client->getClientId(), ERRMSG_FOOTER);
								}

								delete packet;

								/* disconnect the client */
								if ( rc == -1 || rc == -2 )
								{
									packet = new MQTTGWPacket();
									packet->setHeader(DISCONNECT);
									ev = new Event();
									ev->setBrokerRecvEvent(client, packet);
									_gateway->getPacketEventQue()->post(ev);
								}
							}
						}
					}
					client = client->getNextClient();
				}
				_light->blueLight(false);
			}
		}
		else
		{
			_light->greenLight(false);
		}
		maxSock = 0;
	}
}

/**
 *  write message content into stdout or Ringbuffer
 */
int BrokerRecvTask::log(Client* client, MQTTGWPacket* packet)
{
	char pbuf[(SIZE_OF_LOG_PACKET + 5 )* 3];
	char msgId[6];
	int rc = 0;

	switch (packet->getType())
	{
	case CONNACK:
		WRITELOG(FORMAT_Y_Y_W, currentDateTime(), packet->getName(), LEFTARROW, client->getClientId(), packet->print(pbuf));
		break;
	case PUBLISH:
		WRITELOG(FORMAT_W_MSGID_Y_W_NL, currentDateTime(), packet->getName(), packet->getMsgId(msgId), LEFTARROW, client->getClientId(), packet->print(pbuf));
		break;
	case PUBACK:
	case PUBREC:
	case PUBREL:
	case PUBCOMP:
		WRITELOG(FORMAT_W_MSGID_Y_W, currentDateTime(), packet->getName(), packet->getMsgId(msgId), LEFTARROW, client->getClientId(), packet->print(pbuf));
		break;
	case SUBACK:
	case UNSUBACK:
		WRITELOG(FORMAT_W_MSGID_Y_W, currentDateTime(), packet->getName(), packet->getMsgId(msgId), LEFTARROW, client->getClientId(), packet->print(pbuf));
		break;
	case PINGRESP:
		WRITELOG(FORMAT_Y_Y_W, currentDateTime(), packet->getName(), LEFTARROW, client->getClientId(), packet->print(pbuf));
		break;
	default:
		rc = -1;
		break;
	}
	return rc;
}

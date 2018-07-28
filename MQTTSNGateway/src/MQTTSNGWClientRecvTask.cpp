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

#include "MQTTSNGWClientRecvTask.h"
#include "MQTTSNGateway.h"
#include "MQTTSNPacket.h"
#include "MQTTSNGWForwarder.h"
#include "MQTTSNGWEncapsulatedPacket.h"
#include <cstring>

using namespace MQTTSNGW;
char* currentDateTime(void);
/*=====================================
 Class ClientRecvTask
 =====================================*/
ClientRecvTask::ClientRecvTask(Gateway* gateway)
{
	_gateway = gateway;
	_gateway->attach((Thread*)this);
	_sensorNetwork = _gateway->getSensorNetwork();
}

ClientRecvTask::~ClientRecvTask()
{

}

/**
 * Initialize SensorNetwork
 */
void ClientRecvTask::initialize(int argc, char** argv)
{
	if ( _sensorNetwork->initialize() < 0 )
	{
		throw Exception(" Can't open the sensor network.\n");
	}
}

/*
 * Receive a packet from clients via sensor netwwork
 * and generate a event to execute the packet handling  procedure
 * of MQTTSNPacketHandlingTask.
 */
void ClientRecvTask::run()
{
	Event* ev = 0;
	Client* client = 0;
	char buf[128];


	while (true)
	{
	    Forwarder* fwd = 0;
	    WirelessNodeId nodeId;

		MQTTSNPacket* packet = new MQTTSNPacket();
		int packetLen = packet->recv(_sensorNetwork);

		if (CHK_SIGINT)
		{
			WRITELOG("%s ClientRecvTask   stopped.\n", currentDateTime());
			delete packet;
			return;
		}

		if (packetLen < 2 )
		{
			delete packet;
			continue;
		}

		if ( packet->getType() <= MQTTSN_ADVERTISE || packet->getType() == MQTTSN_GWINFO )
		{
			delete packet;
			continue;
		}

		if ( packet->getType() == MQTTSN_SEARCHGW )
		{
			/* write log and post Event */
			log(0, packet);
			ev = new Event();
			ev->setBrodcastEvent(packet);
			_gateway->getPacketEventQue()->post(ev);
			continue;
		}

		if ( packet->getType() == MQTTSN_ENCAPSULATED )
		{
		    fwd = _gateway->getForwarderList()->getForwarder(_sensorNetwork->getSenderAddress());

		    if ( fwd  == 0 )
		    {
		        log(0, packet);
		        WRITELOG("%s Forwarder  %s is not authorized.%s\n", ERRMSG_HEADER, _sensorNetwork->getSenderAddress()->sprint(buf), ERRMSG_FOOTER);
		        delete packet;
		        continue;
		    }
		    else
		    {
		        MQTTSNString fwdName;
		        fwdName.lenstring.data = const_cast<char *>( fwd->getName() );
		        fwdName.lenstring.len = strlen(fwdName.lenstring.data);
	            log(0, packet, &fwdName);

		        MQTTSNGWEncapsulatedPacket  encap;
		        encap.desirialize(packet->getPacketData(), packet->getPacketLength());
		        nodeId.setId( encap.getWirelessNodeId() );
		        client = fwd->getClient(&nodeId);
		        delete packet;
		        packet = encap.getMQTTSNPacket();
		    }
		}
		else
		{
            /* get client from the ClientList of Gateway by sensorNetAddress. */
            client = _gateway->getClientList()->getClient(_sensorNetwork->getSenderAddress());
		}


		if ( client )
		{
			/* write log and post Event */
			log(client, packet);
			ev = new Event();
			ev->setClientRecvEvent(client,packet);
			_gateway->getPacketEventQue()->post(ev);
		}
		else
		{
			/* new client */
 		    if (packet->getType() == MQTTSN_CONNECT)
			{
				MQTTSNPacket_connectData data;
				memset(&data, 0, sizeof(MQTTSNPacket_connectData));
				if ( !packet->getCONNECT(&data) )
				{
					log(0, packet, &data.clientID);
					WRITELOG("%s CONNECT message form %s is incorrect.%s\n", ERRMSG_HEADER, _sensorNetwork->getSenderAddress()->sprint(buf), ERRMSG_FOOTER);
					delete packet;
					continue;
				}

				client = _gateway->getClientList()->getClient(&data.clientID);

				if ( fwd )
				{
				    if ( client == 0 )
				    {
				        /* create a new client */
				        client = _gateway->getClientList()->createClient(0, &data.clientID, false, false);
				    }
				    /* Add to af forwarded client list of forwarder. */
                    fwd->addClient(client, &nodeId);
				}
				else
				{
                    if ( client )
                    {
                        /* Client exists. Set SensorNet Address of it. */
                        client->setClientAddress(_sensorNetwork->getSenderAddress());
                    }
                    else
                    {
                        /* create a new client */
                        client = _gateway->getClientList()->createClient(_sensorNetwork->getSenderAddress(), &data.clientID, false, false);
                    }
				}

				log(client, packet, &data.clientID);

				if (!client)
				{
	                WRITELOG("%s Client(%s) was rejected. CONNECT message has been discarded.%s\n", ERRMSG_HEADER, _sensorNetwork->getSenderAddress()->sprint(buf), ERRMSG_FOOTER);
					delete packet;
					continue;
				}

				/* post Client RecvEvent */
				ev = new Event();
				ev->setClientRecvEvent(client, packet);
				_gateway->getPacketEventQue()->post(ev);
			}
			else
			{
				log(client, packet);
                WRITELOG("%s Client(%s) is not connecting. message has been discarded.%s\n", ERRMSG_HEADER, _sensorNetwork->getSenderAddress()->sprint(buf), ERRMSG_FOOTER);
                delete packet;

                /* Send DISCONNECT */
                if ( fwd == 0 )
                {
                    packet = new MQTTSNPacket();
                    packet->setDISCONNECT(0);
                    ev = new Event();
                    ev->setClientSendEvent(_sensorNetwork->getSenderAddress(), packet);
                    _gateway->getClientSendQue()->post(ev);
                }
			}
		}
	}
}

void ClientRecvTask::log(Client* client, MQTTSNPacket* packet, MQTTSNString* id)
{
	char pbuf[SIZE_OF_LOG_PACKET * 3];
	const char* clientId;
	char cstr[MAX_CLIENTID_LENGTH + 1];
	char msgId[6];

	if ( id )
	{
		memset((void*)cstr, 0, id->lenstring.len + 1);
		strncpy(cstr, id->lenstring.data, id->lenstring.len) ;
		clientId = cstr;
	}
	else if ( client )
	{
		clientId = client->getClientId();
	}
	else
	{
		clientId = UNKNOWNCL;
	}

	switch (packet->getType())
	{
	case MQTTSN_SEARCHGW:
		WRITELOG(FORMAT_Y_G_G_NL, currentDateTime(), packet->getName(), LEFTARROW, CLIENT, packet->print(pbuf));
		break;
	case MQTTSN_CONNECT:
	case MQTTSN_PINGREQ:
		WRITELOG(FORMAT_Y_G_G_NL, currentDateTime(), packet->getName(), LEFTARROW, clientId, packet->print(pbuf));
		break;
	case MQTTSN_DISCONNECT:
	case MQTTSN_WILLTOPICUPD:
	case MQTTSN_WILLMSGUPD:
	case MQTTSN_WILLTOPIC:
	case MQTTSN_WILLMSG:
		WRITELOG(FORMAT_Y_G_G, currentDateTime(), packet->getName(), LEFTARROW, clientId, packet->print(pbuf));
		break;
	case MQTTSN_PUBLISH:
	case MQTTSN_REGISTER:
	case MQTTSN_SUBSCRIBE:
	case MQTTSN_UNSUBSCRIBE:
		WRITELOG(FORMAT_G_MSGID_G_G_NL, currentDateTime(), packet->getName(), packet->getMsgId(msgId), LEFTARROW, clientId, packet->print(pbuf));
		break;
	case MQTTSN_REGACK:
	case MQTTSN_PUBACK:
	case MQTTSN_PUBREC:
	case MQTTSN_PUBREL:
	case MQTTSN_PUBCOMP:
		WRITELOG(FORMAT_G_MSGID_G_G, currentDateTime(), packet->getName(), packet->getMsgId(msgId), LEFTARROW, clientId, packet->print(pbuf));
		break;
	case MQTTSN_ENCAPSULATED:
	        WRITELOG(FORMAT_Y_G_G, currentDateTime(), packet->getName(), LEFTARROW, clientId, packet->print(pbuf));
	        break;
	default:
		WRITELOG(FORMAT_W_NL, currentDateTime(), packet->getName(), LEFTARROW, clientId, packet->print(pbuf));
		break;
	}
}

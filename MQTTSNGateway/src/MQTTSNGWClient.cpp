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
 *    Tieto Poland Sp. z o.o. - Gateway improvements
 **************************************************************************************/

#include "MQTTSNGWDefines.h"
#include "MQTTSNGWClient.h"
#include "MQTTSNGateway.h"
#include "SensorNetwork.h"
#include "Network.h"
#include <string>
#include <string.h>
#include <stdio.h>

#include "MQTTSNGWForwarder.h"

using namespace MQTTSNGW;
char* currentDateTime(void);
/*=====================================
 Class ClientList
 =====================================*/
ClientList::ClientList()
{
	_clientCnt = 0;
	_authorize = false;
	_firstClient = 0;
	_endClient = 0;
}

ClientList::~ClientList()
{
	_mutex.lock();
	Client* cl = _firstClient;
	Client* ncl;

	while (cl != 0)
	{
		ncl = cl->_nextClient;
		delete cl;
		cl = ncl;
	};
	_mutex.unlock();
}

/**
 * Create ClientList from a client list file.
 * @param File name of the client list
 * @return true: Reject client connection that is not registered in the client list
 *
 * File format is:
 *     Lines bigning with # are comment line.
 *     ClientId, SensorNetAddress, "unstableLine", "secureConnection"
 *     in case of UDP, SensorNetAddress format is portNo@IPAddress.
 *     if the SensorNetwork is not stable, write unstableLine.
 *     if BrokerConnection is SSL, write secureConnection.
 *
 * Ex:
 *     #Client List
 *     ClientId1,11200@192.168.10.10
 *     ClientID2,35000@192.168.50.200,unstableLine
 *     ClientID3,40000@192.168.200.50,secureConnection
 *     ClientID4,41000@192.168.200.51,unstableLine,secureConnection
 */
bool ClientList::authorize(const char* fileName)
{
	FILE* fp;
	char buf[MAX_CLIENTID_LENGTH + 256];
	size_t pos;
	bool secure;
	bool stable;
	SensorNetAddress netAddr;
	MQTTSNString clientId = MQTTSNString_initializer;

	if ((fp = fopen(fileName, "r")) != 0)
	{
		while (fgets(buf, MAX_CLIENTID_LENGTH + 254, fp) != 0)
		{
			if (*buf == '#')
			{
				continue;
			}
			string data = string(buf);
			while ((pos = data.find_first_of(" 　\t\n")) != string::npos)
			{
				data.erase(pos, 1);
			}
			if (data.empty())
			{
				continue;
			}
			pos = data.find_first_of(",");
			string id = data.substr(0, pos);
			clientId.cstring = strdup(id.c_str());
			string addr = data.substr(pos + 1);

			if (netAddr.setAddress(&addr) == 0)
			{
				secure = (data.find("secureConnection") != string::npos);
				stable = !(data.find("unstableLine") != string::npos);
				createClient(&netAddr, &clientId, stable, secure);
			}
			else
			{
				WRITELOG("Invalid address     %s\n", data.c_str());
			}
			free(clientId.cstring);
		}
		fclose(fp);
		_authorize = true;
	}
	return _authorize;
}

bool ClientList::setPredefinedTopics(const char* fileName)
{
    FILE* fp;
    char buf[MAX_CLIENTID_LENGTH + 256];
    size_t pos0, pos1;
    MQTTSNString clientId = MQTTSNString_initializer;;
    bool rc = false;

    if ((fp = fopen(fileName, "r")) != 0)
    {
        while (fgets(buf, MAX_CLIENTID_LENGTH + 254, fp) != 0)
        {
            if (*buf == '#')
            {
                continue;
            }
            string data = string(buf);
            while ((pos0 = data.find_first_of(" 　\t\n")) != string::npos)
            {
                data.erase(pos0, 1);
            }
            if (data.empty())
            {
                continue;
            }

            pos0 = data.find_first_of(",");
            pos1 = data.find(",", pos0 + 1) ;
            string id = data.substr(0, pos0);
            clientId.cstring = strdup(id.c_str());
            string topicName = data.substr(pos0 + 1, pos1 - pos0 -1);
            uint16_t topicID = stoul(data.substr(pos1 + 1));
            createPredefinedTopic( &clientId, topicName,  topicID);
            free(clientId.cstring);
        }
        fclose(fp);
        rc = true;
    }
    else
    {
        WRITELOG("Can not open the Predefined Topic List.     %s\n", fileName);
        return false;
    }
    return rc;
}

void ClientList::erase(Client*& client)
{
	if ( !_authorize && client->erasable())
	{
		_mutex.lock();
		Client* prev = client->_prevClient;
		Client* next = client->_nextClient;

		if (prev)
		{
			prev->_nextClient = next;
		}
		else
		{
			_firstClient = next;

		}
		if (next)
		{
			next->_prevClient = prev;
		}
		else
		{
			_endClient = prev;
		}
		_clientCnt--;
		Forwarder* fwd = client->getForwarder();
		if ( fwd )
		{
		    fwd->eraseClient(client);
		}
		delete client;
		client = 0;
		_mutex.unlock();
	}
}

Client* ClientList::getClient(SensorNetAddress* addr)
{
    if ( addr )
    {
        _mutex.lock();
        Client* client = _firstClient;

        while (client != 0)
        {
            if (client->getSensorNetAddress()->isMatch(addr) )
            {
                _mutex.unlock();
                return client;
            }
            client = client->_nextClient;
        }
        _mutex.unlock();
    }
	return 0;
}

Client* ClientList::getClient(void)
{
	return _firstClient;
}


Client* ClientList::getClient(MQTTSNString* clientId)
{
	_mutex.lock();
	Client* client = _firstClient;
	const char* clID =clientId->cstring;

	if (clID == 0 )
	{
	    clID = clientId->lenstring.data;
	}

	while (client != 0)
	{
		if (strncmp((const char*)client->getClientId(), clID, MQTTSNstrlen(*clientId)) == 0 )
		{
			_mutex.unlock();
			return client;
		}
		client = client->_nextClient;
	}
	_mutex.unlock();
	return 0;
}

Client* ClientList::createClient(SensorNetAddress* addr, MQTTSNString* clientId, bool unstableLine, bool secure)
{
	Client* client = 0;

	/* clients must be authorized */
	if ( _authorize )
	{
		/* search cliene with sensorNetAddress from the list */
		return getClient(addr);
	}

	/*  anonimous clients */
	if ( _clientCnt > MAX_CLIENTS )
	{
		return 0;  // full of clients
	}

	client = getClient(addr);
	if ( client )
	{
		return client;
	}

	/* creat a new client */
	client = new Client(secure);
	if ( addr )
	{
	    client->setClientAddress(addr);
	}
	client->setSensorNetType(unstableLine);
	if ( MQTTSNstrlen(*clientId) )
	{
		client->setClientId(*clientId);
	}
	else
	{
		MQTTSNString  dummyId MQTTSNString_initializer;;
		dummyId.cstring = strdup("");
		client->setClientId(dummyId);
		 free(dummyId.cstring);
	}

	_mutex.lock();

	/* add the list */
	if ( _firstClient == 0 )
	{
		_firstClient = client;
		_endClient = client;
	}
	else
	{
		_endClient->_nextClient = client;
		client->_prevClient = _endClient;
		_endClient = client;
	}
	_clientCnt++;
	_mutex.unlock();
	return client;
}

Client* ClientList::createPredefinedTopic( MQTTSNString* clientId, string topicName, uint16_t topicId)
{
    Client* client = getClient(clientId);

    if ( _authorize && client == 0)
    {
        return 0;
    }

    /*  anonimous clients */
    if ( _clientCnt > MAX_CLIENTS )
    {
        return 0;  // full of clients
    }

    if ( client == 0 )
    {
        /* creat a new client */
        client = new Client();
        client->setClientId(*clientId);

        _mutex.lock();

        /* add the list */
        if ( _firstClient == 0 )
        {
            _firstClient = client;
            _endClient = client;
        }
        else
        {
            _endClient->_nextClient = client;
            client->_prevClient = _endClient;
            _endClient = client;
        }
        _clientCnt++;
        _mutex.unlock();
    }

    // create Topic & Add it
    client->getTopics()->add((const char*)topicName.c_str(), topicId);
    client->_hasPredefTopic = true;
    return client;
}

uint16_t ClientList::getClientCount()
{
	return _clientCnt;
}

bool ClientList::isAuthorized()
{
	return _authorize;
}

/*=====================================
 Class Client
 =====================================*/
static const char* theClientStatus[] = { "Disconnected", "TryConnecting", "Connecting", "Active", "Asleep", "Awake", "Lost" };

Client::Client(bool secure)
{
	_packetId = 0;
	_snMsgId = 0;
	_status = Cstat_Disconnected;
	_keepAliveMsec = 0;
	_topics = new Topics();
	_clientId = 0;
	_willTopic = 0;
	_willMsg = 0;
	_connectData.Protocol = 0;
	_connectData.clientID = 0;
	_connectData.flags.all = 0;
	_connectData.header.byte = 0;
	_connectData.keepAliveTimer = 0;
	_connectData.version = 0;
	_connectData.willMsg = 0;
	_connectData.willTopic = 0;
	_network = new Network(secure);
	_secureNetwork = secure;
	_sensorNetype = true;
	_connAck = 0;
	_waitWillMsgFlg = false;
	_sessionStatus = false;
	_prevClient = 0;
	_nextClient = 0;
	_clientSleepPacketQue.setMaxSize(MAX_SAVED_PUBLISH);
	_proxyPacketQue.setMaxSize(MAX_SAVED_PUBLISH);
	_hasPredefTopic = false;
	_holdPingRequest = false;
	_forwarder = 0;
	_isProxy = false;
}

Client::~Client()
{
	if ( _topics )
	{
		delete _topics;
	}

	if ( _clientId )
	{
		free(_clientId);
	}

	if ( _willTopic )
	{
		free(_willTopic);
	}

	if ( _willMsg )
	{
		free(_willMsg);
	}

	if (_connAck)
	{
		delete _connAck;
	}

	if (_network)
	{
		delete _network;
	}
}

TopicIdMapelement* Client::getWaitedPubTopicId(uint16_t msgId)
{
	return _waitedPubTopicIdMap.getElement(msgId);
}

TopicIdMapelement* Client::getWaitedSubTopicId(uint16_t msgId)
{
	return _waitedSubTopicIdMap.getElement(msgId);
}

MQTTGWPacket* Client::getClientSleepPacket()
{
	return _clientSleepPacketQue.getPacket();
}

void Client::deleteFirstClientSleepPacket()
{
	_clientSleepPacketQue.pop();
}

int Client::setClientSleepPacket(MQTTGWPacket* packet)
{
	int rc = _clientSleepPacketQue.post(packet);
	if ( rc )
	{
		WRITELOG("%s    %s is sleeping. the packet was saved.\n", currentDateTime(), _clientId);
	}
	else
	{
		WRITELOG("%s    %s is sleeping　but discard the packet.\n", currentDateTime(), _clientId);
	}
	return rc;
}

MQTTSNPacket* Client::getProxyPacket(void)
{
    return _proxyPacketQue.getPacket();
}

void Client::deleteFirstProxyPacket()
{
    _proxyPacketQue.pop();
}

int Client::setProxyPacket(MQTTSNPacket* packet)
{
    int rc = _proxyPacketQue.post(packet);
    if ( rc )
    {
        WRITELOG("%s    %s is Disconnected. the packet was saved.\n", currentDateTime(), _clientId);
    }
    else
    {
        WRITELOG("%s    %s is Disconnected and discard the packet.\n", currentDateTime(), _clientId);
    }
    return rc;
}

Connect* Client::getConnectData(void)
{
	return &_connectData;
}

void Client::eraseWaitedPubTopicId(uint16_t msgId)
{
	_waitedPubTopicIdMap.erase(msgId);
}

void Client::eraseWaitedSubTopicId(uint16_t msgId)
{
	_waitedSubTopicIdMap.erase(msgId);
}

void Client::clearWaitedPubTopicId(void)
{
	_waitedPubTopicIdMap.clear();
}

void Client::clearWaitedSubTopicId(void)
{
	_waitedSubTopicIdMap.clear();
}

void Client::setWaitedPubTopicId(uint16_t msgId, uint16_t topicId, MQTTSN_topicTypes type)
{
	_waitedPubTopicIdMap.add(msgId, topicId, type);
}
void Client::setWaitedSubTopicId(uint16_t msgId, uint16_t topicId, MQTTSN_topicTypes type)
{
	_waitedSubTopicIdMap.add(msgId, topicId, type);
}

bool Client::checkTimeover(void)
{
	return (_status == Cstat_Active && _keepAliveTimer.isTimeup());
}

void Client::setKeepAlive(MQTTSNPacket* packet)
{
	MQTTSNPacket_connectData param;
	if (packet->getCONNECT(&param))
	{
		_keepAliveMsec = param.duration * 1000UL;
		_keepAliveTimer.start(_keepAliveMsec * 1.5);
	}
}

void Client::setForwarder(Forwarder* forwarder)
{
    _forwarder = forwarder;
}

Forwarder* Client::getForwarder(void)
{
    return _forwarder;
}

void Client::setSessionStatus(bool status)
{
	_sessionStatus = status;
}

bool Client::erasable(void)
{
	return _sessionStatus && !_hasPredefTopic  && _forwarder == 0;
}

void Client::updateStatus(MQTTSNPacket* packet)
{
	if (((_status == Cstat_Disconnected) || (_status == Cstat_Lost)) && packet->getType() == MQTTSN_CONNECT)
	{
		setKeepAlive(packet);
	}
	else if (_status == Cstat_Active)
	{
		switch (packet->getType())
		{
		case MQTTSN_PINGREQ:
		case MQTTSN_PUBLISH:
		case MQTTSN_SUBSCRIBE:
		case MQTTSN_UNSUBSCRIBE:
		case MQTTSN_PUBACK:
		case MQTTSN_PUBCOMP:
		case MQTTSN_PUBREL:
		case MQTTSN_PUBREC:
			if ( !_isProxy )
			{
			    _keepAliveTimer.start(_keepAliveMsec * 1.5);
			}
			break;
		case MQTTSN_DISCONNECT:
			uint16_t duration;
			packet->getDISCONNECT(&duration);
			if (duration)
			{
				_status = Cstat_Asleep;
			}
			else
			{
				disconnected();
			}
			break;
		default:
			break;
		}
	}
	else if (_status == Cstat_Awake || _status == Cstat_Asleep)
	{
		switch (packet->getType())
		{
		case MQTTSN_CONNECT:
			_status = Cstat_Active;
			break;
		case MQTTSN_DISCONNECT:
			disconnected();
			break;
		case MQTTSN_PINGREQ:
			_status = Cstat_Awake;
			break;
		case MQTTSN_PINGRESP:
			_status = Cstat_Asleep;
			break;
		default:
			break;
		}
	}
	DEBUGLOG("Client Status = %s\n", theClientStatus[_status]);
}

void Client::updateStatus(ClientStatus stat)
{
	_status = stat;
}

void Client::connectSended()
{
	_status = Cstat_Connecting;
}

void Client::connackSended(int rc)
{
	if (rc == MQTTSN_RC_ACCEPTED)
	{
		_status = Cstat_Active;
	}
	else
	{
		disconnected();
	}
}

void Client::disconnected(void)
{
	_status = Cstat_Disconnected;
	_waitWillMsgFlg = false;
}

void Client::tryConnect(void)
{
    _status = Cstat_TryConnecting;
}

bool Client::isConnectSendable(void)
{
	if ( _status == Cstat_Lost || _status == Cstat_TryConnecting )
	{
		return false;
	}
	else
	{
		return true;
	}
}

uint16_t Client::getNextPacketId(void)
{
	_packetId++;
	if ( _packetId == 0xffff )
	{
		_packetId = 1;
	}
	return _packetId;
}

uint8_t Client::getNextSnMsgId(void)
{
	_snMsgId++;
	if (_snMsgId == 0)
	{
		_snMsgId++;
	}
	return _snMsgId;
}

Topics* Client::getTopics(void)
{
	return _topics;
}

Network* Client::getNetwork(void)
{
	return _network;
}

void Client::setClientAddress(SensorNetAddress* sensorNetAddr)
{
	_sensorNetAddr = *sensorNetAddr;
}

SensorNetAddress* Client::getSensorNetAddress(void)
{
	return &_sensorNetAddr;
}

void Client::setSensorNetType(bool stable)
{
	_sensorNetype = stable;
}

void Client::setTopics(Topics* topics)
{
  _topics = topics;
}

ClientStatus Client::getClientStatus(void)
{
    return _status;
}

void Client::setWaitWillMsgFlg(bool flg)
{
	_waitWillMsgFlg = flg;
}

bool Client::isWaitWillMsg(void)
{
	return _waitWillMsgFlg;
}

bool Client::isDisconnect(void)
{
	return (_status == Cstat_Disconnected);
}

bool Client::isActive(void)
{
	return (_status == Cstat_Active);
}

bool Client::isSleep(void)
{
	return (_status == Cstat_Asleep);
}

bool Client::isAwake(void)
{
	return (_status == Cstat_Awake);
}

bool Client::isConnecting(void)
{
    return (_status == Cstat_Connecting);
}

bool Client::isSecureNetwork(void)
{
	return _secureNetwork;
}

bool Client::isSensorNetStable(void)
{
	return _sensorNetype;
}

WaitREGACKPacketList* Client::getWaitREGACKPacketList()
{
	return &_waitREGACKList;
}

Client* Client::getNextClient(void)
{
	return _nextClient;
}

void Client::setClientId(MQTTSNString id)
{
	if ( _clientId )
	{
		free(_clientId);
	}

	if ( id.cstring )
	{
	    _clientId = (char*)calloc(strlen(id.cstring) + 1, 1);
	    memcpy(_clientId, id.cstring, strlen(id.cstring));
	}
	else
	{
        /* save clientId into (char*)_clientId NULL terminated */
        _clientId = (char*)calloc(MQTTSNstrlen(id) + 1, 1);
        unsigned char* ptr = (unsigned char*)_clientId;
        writeMQTTSNString((unsigned char**)&ptr, id);
	}
}

void Client::setWillTopic(MQTTSNString willTopic)
{
	if ( _willTopic )
	{
		free(_willTopic);
	}

	_willTopic = (char*)calloc(MQTTSNstrlen(willTopic) + 1, 1);
	/* save willTopic into (char*)_willTopic  with NULL termination */
	unsigned char* ptr = (unsigned char*)_willTopic;
	writeMQTTSNString((unsigned char**)&ptr, willTopic);
}

void Client::setWillMsg(MQTTSNString willMsg)
{
	if ( _willMsg)
	{
		free(_willMsg);
	}

	_willMsg = (char*)calloc(MQTTSNstrlen(willMsg) + 1, 1);
	/* save willMsg into (char*)_willMsg  with NULL termination */
	unsigned char* ptr = (unsigned char*)_willMsg;
	writeMQTTSNString((unsigned char**)&ptr, willMsg);
}

char* Client::getClientId(void)
{
	return _clientId;
}

char* Client::getWillTopic(void)
{
	return _willTopic;
}

char* Client::getWillMsg(void)
{
	return _willMsg;
}

const char* Client::getStatus(void)
{
	return theClientStatus[_status];
}

bool Client::isProxy(void)
{
	return _isProxy;
}

void Client::setPorxy(bool isProxy)
{
     _isProxy = isProxy;;
}

void Client::holdPingRequest(void)
{
    _holdPingRequest = true;
}

void Client::resetPingRequest(void)
{
    _holdPingRequest = false;
}

bool Client::isHoldPringReqest(void)
{
    return _holdPingRequest;
}

/*=====================================
 Class Topic
 ======================================*/
Topic::Topic()
{
    _type = MQTTSN_TOPIC_TYPE_NORMAL;
	_topicName = 0;
	_topicId = 0;
	_next = 0;
}

Topic::Topic(string* topic, MQTTSN_topicTypes type)
{
    _type = type;
	_topicName = topic;
	_topicId = 0;
	_next = 0;
}

Topic::~Topic()
{
	if ( _topicName )
	{
		delete _topicName;
	}
}

string* Topic::getTopicName(void)
{
	return _topicName;
}

uint16_t Topic::getTopicId(void)
{
	return _topicId;
}

MQTTSN_topicTypes Topic::getType(void)
{
    return _type;
}

bool Topic::isMatch(string* topicName)
{
	string::size_type tlen = _topicName->size();

	string::size_type tpos = 0;
	string::size_type tloc = 0;
	string::size_type pos = 0;
	string::size_type loc = 0;
	string wildcard = "#";
	string wildcards = "+";

	while(true)
	{
		loc = topicName->find('/', pos);
		tloc = _topicName->find('/', tpos);

		if ( loc != string::npos && tloc != string::npos )
		{
			string subtopic = topicName->substr(pos, loc - pos);
			string subtopict = _topicName->substr(tpos, tloc - tpos);
			if (subtopict == wildcard)
			{
				return true;
			}
			else if (subtopict == wildcards)
			{
				if ( (tpos = tloc + 1 ) > tlen )
				{
					pos = loc + 1;
					loc = topicName->find('/', pos);
					if ( loc == string::npos )
					{
						return true;
					}
					else
					{
						return false;
					}
				}
				pos = loc + 1;
			}
			else if ( subtopic != subtopict )
			{
				return false;
			}
			else
			{
				if ( (tpos = tloc + 1) > tlen )
				{
					return false;
				}

				pos = loc + 1;
			}
		}
		else if ( loc == string::npos && tloc == string::npos )
		{
			string subtopic = topicName->substr(pos);
			string subtopict = _topicName->substr(tpos);
			if ( subtopict == wildcard || subtopict == wildcards)
			{
				return true;
			}
			else if ( subtopic == subtopict )
			{
				return true;
			}
			else
			{
				return false;
			}
		}
		else if ( loc == string::npos && tloc != string::npos )
		{
			string subtopic = topicName->substr(pos);
			string subtopict = _topicName->substr(tpos, tloc - tpos);
			if ( subtopic != subtopict)
			{
				return false;
			}

			tpos = tloc + 1;

			return _topicName->substr(tpos) == wildcard;
		}
		else if ( loc != string::npos && tloc == string::npos )
		{
			return _topicName->substr(tpos) == wildcard;
		}
	}
}

void Topic::print(void)
{
    WRITELOG("TopicName=%s  ID=%d  Type=%d\n", _topicName->c_str(), _topicId, _type);
}

/*=====================================
 Class Topics
 ======================================*/
Topics::Topics()
{
	_first = 0;
	_nextTopicId = 0;
	_cnt = 0;
}

Topics::~Topics()
{
	Topic* p = _first;
	while (p)
	{
		Topic* q = p->_next;
		delete p;
		p = q;
	}
}

Topic* Topics::getTopicByName(const MQTTSN_topicid* topicid)
{
    Topic* p = _first;
    char* ch = topicid->data.long_.name;

    string sname = string(ch, ch + topicid->data.long_.len);
    while (p)
    {
         if (  p->_topicName->compare(sname) == 0 )
         {
             return p;
         }
         p = p->_next;
    }
    return 0;
}

Topic* Topics::getTopicById(const MQTTSN_topicid* topicid)
{
	Topic* p = _first;

	while (p)
	{
	    if ( p->_type == topicid->type && p->_topicId == topicid->data.id )
	    {
	        return p;
	    }
		p = p->_next;
	}
	return 0;
}

// For MQTTSN_TOPIC_TYPE_NORMAL */
Topic* Topics::add(const MQTTSN_topicid* topicid)
{
	if (topicid->type != MQTTSN_TOPIC_TYPE_NORMAL )
	{
	    return 0;
	}

	Topic* topic = getTopicByName(topicid);

	if ( topic )
    {
	    return topic;
    }
	string name(topicid->data.long_.name, topicid->data.long_.len);
	return add(name.c_str(), 0);
}

Topic* Topics::add(const char* topicName, uint16_t id)
{
    MQTTSN_topicid topicId;

    if (  _cnt >= MAX_TOPIC_PAR_CLIENT )
    {
        return 0;
    }

    topicId.data.long_.name = (char*)const_cast<char*>(topicName);
    topicId.data.long_.len = strlen(topicName);


    Topic* topic = getTopicByName(&topicId);

    if ( topic )
    {
        return topic;
    }

    topic = new Topic();

    if (topic == 0)
    {
        return 0;
    }

    string* name = new string(topicName);
    topic->_topicName = name;

    if ( id == 0 )
    {
        topic->_type = MQTTSN_TOPIC_TYPE_NORMAL;
        topic->_topicId = getNextTopicId();
    }
    else
    {
        topic->_type = MQTTSN_TOPIC_TYPE_PREDEFINED;
        topic->_topicId  = id;
    }

    _cnt++;

    if ( _first == 0)
    {
        _first = topic;
    }
    else
    {
        Topic* tp = _first;
        while (tp)
        {
            if (tp->_next == 0)
            {
                tp->_next = topic;
                break;
            }
            else
            {
                tp = tp->_next;
            }
        }
    }
    return topic;
}

uint16_t Topics::getNextTopicId()
{
	return ++_nextTopicId == 0xffff ? _nextTopicId += 2 : _nextTopicId;
}

Topic* Topics::match(const MQTTSN_topicid* topicid)
{
	if (topicid->type != MQTTSN_TOPIC_TYPE_NORMAL)
	{
		return 0;
	}
	string topicName(topicid->data.long_.name, topicid->data.long_.len);

	Topic* topic = _first;
	while (topic)
	{
		if (topic->isMatch(&topicName))
		{
			return topic;
		}
		topic = topic->_next;
	}
	return 0;
}


void Topics::eraseNormal(void)
{
    Topic* topic = _first;
    Topic* next = 0;
    Topic* prev = 0;

    while (topic)
    {
        if ( topic->_type == MQTTSN_TOPIC_TYPE_NORMAL )
        {
            next = topic->_next;
            if ( _first == topic )
            {
                _first = next;
            }
            if ( prev  )
            {
                prev->_next = next;
            }
            delete topic;
            _cnt--;
            topic = next;
        }
        else
        {
            prev = topic;
            topic = topic->_next;
        }
    }
}

void Topics::print(void)
{
    Topic* topic = _first;
    if (topic == 0 )
    {
        WRITELOG("No Topic.\n");
    }
    else
    {
        while (topic)
        {
            topic->print();
            topic = topic->_next;
        }
    }
}

uint8_t Topics::getCount(void)
{
    return _cnt;
}

/*=====================================
 Class TopicIdMap
 =====================================*/
TopicIdMapelement::TopicIdMapelement(uint16_t msgId, uint16_t topicId, MQTTSN_topicTypes type)
{
	_msgId = msgId;
	_topicId = topicId;
	_type = type;
	_next = 0;
	_prev = 0;
}

TopicIdMapelement::~TopicIdMapelement()
{

}

MQTTSN_topicTypes TopicIdMapelement::getTopicType(void)
{
    return _type;
}

uint16_t TopicIdMapelement::getTopicId(void)
{
    return  _topicId;
}

TopicIdMap::TopicIdMap()
{
	_maxInflight = MAX_INFLIGHTMESSAGES;
	_msgIds = 0;
	_first = 0;
	_end = 0;
	_cnt = 0;
}

TopicIdMap::~TopicIdMap()
{
	TopicIdMapelement* p = _first;
	while ( p )
	{
		TopicIdMapelement* q = p->_next;
		delete p;
		p = q;
	}
}

TopicIdMapelement* TopicIdMap::getElement(uint16_t msgId)
{
	TopicIdMapelement* p = _first;
	while ( p )
	{
		if ( p->_msgId == msgId )
		{
			return p;
		}
		p = p->_next;
	}
	return 0;
}

TopicIdMapelement* TopicIdMap::add(uint16_t msgId, uint16_t topicId, MQTTSN_topicTypes type)
{
	if ( _cnt > _maxInflight * 2 || ( topicId == 0 && type != MQTTSN_TOPIC_TYPE_SHORT ) )
	{
		return 0;
	}
	if ( getElement(msgId) > 0 )
	{
		erase(msgId);
	}

	TopicIdMapelement* elm = new TopicIdMapelement(msgId, topicId, type);
	if ( elm == 0 )
	{
		return 0;
	}
	if ( _first == 0 )
	{
		_first = elm;
		_end = elm;
	}
	else
	{
		elm->_prev = _end;
		_end->_next = elm;
		_end = elm;
	}
	_cnt++;
	return elm;
}

void TopicIdMap::erase(uint16_t msgId)
{
	TopicIdMapelement* p = _first;
	while ( p )
	{
		if ( p->_msgId == msgId )
		{
			if ( p->_prev == 0 )
			{
				_first = p->_next;
			}
			else
			{
				p->_prev->_next = p->_next;
			}

			if ( p->_next == 0 )
			{
				_end = p->_prev;
			}
			else
			{
				p->_next->_prev = p->_prev;
			}
			delete p;
			break;

		}
		p = p->_next;
	}
	_cnt--;
}

void TopicIdMap::clear(void)
{
	TopicIdMapelement* p = _first;
	while ( p )
	{
		TopicIdMapelement* q = p->_next;
		delete p;
		p = q;
	}
	_first = 0;
	_end = 0;
	_cnt = 0;
}

/*=====================================
 Class WaitREGACKPacket
 =====================================*/
waitREGACKPacket::waitREGACKPacket(MQTTSNPacket* packet, uint16_t REGACKMsgId)
{
	_packet = packet;
	_msgId = REGACKMsgId;
	_next = 0;
	_prev = 0;
}

waitREGACKPacket::~waitREGACKPacket()
{
	delete _packet;
}

/*=====================================
 Class WaitREGACKPacketList
 =====================================*/

WaitREGACKPacketList::WaitREGACKPacketList()
{
	_first = 0;
	_end = 0;
	_cnt = 0;
}

WaitREGACKPacketList::~WaitREGACKPacketList()
{
	waitREGACKPacket* p = _first;
	while (p)
	{
		waitREGACKPacket* q = p->_next;
		delete p;
		p = q;
	}
}

int WaitREGACKPacketList::setPacket(MQTTSNPacket* packet, uint16_t REGACKMsgId)
{
	waitREGACKPacket* elm = new waitREGACKPacket(packet, REGACKMsgId);
	if (elm == 0)
	{
		return 0;
	}

	if (_first == 0)
	{
		_first = elm;
		_end = elm;
	}
	else
	{
		_end->_next = elm;
		elm->_prev = _end;
		_end = elm;
	}
	_cnt++;
	return 1;
}

MQTTSNPacket* WaitREGACKPacketList::getPacket(uint16_t REGACKMsgId)
{
	waitREGACKPacket* p = _first;
	while (p)
	{
		if (p->_msgId == REGACKMsgId)
		{
			return p->_packet;
		}
		p = p->_next;
	}
	return 0;
}

void WaitREGACKPacketList::erase(uint16_t REGACKMsgId)
{
	waitREGACKPacket* p = _first;
	while (p)
	{
		if (p->_msgId == REGACKMsgId)
		{
			if (p->_prev == 0)
			{
				_first = p->_next;

			}
			else
			{
				p->_prev->_next = p->_next;
			}
			if (p->_next == 0)
			{
				_end = p->_prev;
			}
			else
			{
				p->_next->_prev = p->_prev;
			}
			_cnt--;
            break;
            // Do not delete element. Element is deleted after sending to Client.
		}
		p = p->_next;
	}
}

uint8_t WaitREGACKPacketList::getCount(void)
{
    return _cnt;
}



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

#ifndef MQTTSNGWCLIENT_H_
#define MQTTSNGWCLIENT_H_

#include <Timer.h>    // Timer class
#include "MQTTSNGWProcess.h"
#include "MQTTGWPacket.h"
#include "MQTTSNGWPacket.h"
#include "MQTTSNPacket.h"
#include "Network.h"
#include "SensorNetwork.h"
#include "MQTTSNPacket.h"

namespace MQTTSNGW
{

#define MQTTSN_TOPIC_MULTI_WILDCARD   '#'
#define MQTTSN_TOPIC_SINGLE_WILDCARD  '+'

/*=====================================
 Class PacketQue
 =====================================*/
template<class T> class PacketQue
{
public:
	PacketQue()
	{
		_que = new Que<T>;
	}

	~PacketQue()
	{
		clear();
		delete _que;
	}

	T* getPacket()
	{
		T* packet;
		if (_que->size() > 0)
		{
			_mutex.lock();
			packet = _que->front();
			_mutex.unlock();
			return packet;
		}
		else
		{
			return 0;
		}
	}

	void post(T* packet)
	{
		_mutex.lock();
		_que->post(packet);
		_mutex.unlock();
	}

	void pop()
	{
		if (_que->size() > 0)
		{
			_mutex.lock();
			_que->pop();
			_mutex.unlock();
		}
	}

	void clear()
	{
		_mutex.lock();
		while (_que->size() > 0)
		{
			delete _que->front();
			_que->pop();
		}
		_mutex.unlock();
	}

private:
	Que<T>* _que;
	Mutex _mutex;
};


/*=====================================
 Class Topic
 ======================================*/
class Topic
{
	friend class Topics;
public:
	Topic();
	Topic(string* topic);
	~Topic();

	string* getTopicName(void);
	uint16_t getTopicId(void);
	int hasWildCard(unsigned int* pos);
	bool isMatch(string* topicName);

private:
	uint16_t _topicId;
	string*  _topicName;
	Topic* _next;
};

/*=====================================
 Class Topics
 ======================================*/
class Topics
{
public:
	Topics();
	~Topics();
	Topic* add(MQTTSN_topicid* topicid);
	Topic* add(string* topic);
	uint16_t getTopicId(MQTTSN_topicid* topic);
	uint16_t getNextTopicId();
	Topic* getTopic(uint16_t topicId);
	Topic* getTopic(MQTTSN_topicid* topicid);
	Topic* match(MQTTSN_topicid* topicid);

private:
	uint16_t _nextTopicId;
	Topic* _first;

};

/*=====================================
 Class TopicIdMap
 =====================================*/
class TopicIdMapelement
{
	friend class TopicIdMap;
public:
	TopicIdMapelement(uint16_t msgId, uint16_t topicId, MQTTSN_topicTypes type);
	~TopicIdMapelement();

private:
	uint16_t _msgId;
	uint16_t _topicId;
	MQTTSN_topicTypes _type;
	TopicIdMapelement* _next;
	TopicIdMapelement* _prev;
};

class TopicIdMap
{
public:
	TopicIdMap();
	~TopicIdMap();
	uint16_t getTopicId(uint16_t msgId, MQTTSN_topicTypes* type);
	Topic* getTopic(MQTTSN_topicTypes type);
	int add(uint16_t msgId, uint16_t topicId, MQTTSN_topicTypes type);
	void erase(uint16_t msgId);
	void clear(void);
private:
	int find(uint16_t msgId);
	uint16_t* _msgIds;
	TopicIdMapelement* _first;
	TopicIdMapelement* _end;
	int _cnt;
	int _maxInflight;
};

/*=====================================
 Class WaitREGACKPacket
 =====================================*/
class waitREGACKPacket
{
	friend class WaitREGACKPacketList;
public:
	waitREGACKPacket(MQTTSNPacket* packet, uint16_t REGACKMsgId);
	~waitREGACKPacket();

private:
	uint16_t _msgId;
	MQTTSNPacket* _packet;
	waitREGACKPacket* _next;
	waitREGACKPacket* _prev;
};

/*=====================================
 Class WaitREGACKPacketList
 =====================================*/
class WaitREGACKPacketList
{
public:
	WaitREGACKPacketList();
	~WaitREGACKPacketList();
	int setPacket(MQTTSNPacket* packet, uint16_t REGACKMsgId);
	MQTTSNPacket* getPacket(uint16_t REGACKMsgId);
	void erase(uint16_t REGACKMsgId);

private:
	waitREGACKPacket* _first;
	waitREGACKPacket* _end;
};

/*=====================================
 Class Client
 =====================================*/
#define MQTTSN_CLIENTID_LENGTH 23

typedef enum
{
	Cstat_Disconnected = 0, Cstat_TryConnecting, Cstat_Connecting, Cstat_Active, Cstat_Asleep, Cstat_Awake, Cstat_Lost
} ClientStatus;


class Client
{
	friend class ClientList;
public:
	Client(bool secure = false);
	Client(uint8_t maxInflightMessages, bool secure);
	~Client();

	Connect* getConnectData(void);
	uint16_t getWaitedPubTopicId(uint16_t msgId);
	uint16_t getWaitedSubTopicId(uint16_t msgId);
	MQTTSNPacket* getClientSleepPacket();
	WaitREGACKPacketList* getWaitREGACKPacketList(void);

	void eraseWaitedPubTopicId(uint16_t msgId);
	void eraseWaitedSubTopicId(uint16_t msgId);
	void clearWaitedPubTopicId(void);
	void clearWaitedSubTopicId(void);

	void setClientSleepPacket(MQTTSNPacket*);
	void setWaitedPubTopicId(uint16_t msgId, uint16_t topicId, MQTTSN_topicTypes type);
	void setWaitedSubTopicId(uint16_t msgId, uint16_t topicId, MQTTSN_topicTypes type);

	void checkTimeover(void);
	void updateStatus(MQTTSNPacket*);
	void updateStatus(ClientStatus);
	void connectSended(void);
	void connackSended(int rc);
	void disconnected(void);
	bool isConnectSendable(void);

	uint16_t getNextPacketId(void);
	uint8_t getNextSnMsgId(void);
	Topics* getTopics(void);
	void setTopics(Topics* topics);
	void setKeepAlive(MQTTSNPacket* packet);

	SensorNetAddress* getSensorNetAddress(void);
	Network* getNetwork(void);
	void setClientAddress(SensorNetAddress* sensorNetAddr);
	void setSensorNetType(bool stable);

	void setClientId(MQTTSNString id);
	void setWillTopic(MQTTSNString willTopic);
	void setWillMsg(MQTTSNString willmsg);
	char* getClientId(void);
	char* getWillTopic(void);
	char* getWillMsg(void);
	const char* getStatus(void);
	void setWaitWillMsgFlg(bool);

	bool isDisconnect(void);
	bool isActive(void);
	bool isSleep(void);
	bool isSecureNetwork(void);
	bool isSensorNetStable(void);
	bool isWaitWillMsg(void);

	Client* getNextClient(void);

private:
	PacketQue<MQTTSNPacket> _clientSleepPacketQue;
	WaitREGACKPacketList    _waitREGACKList;

	Topics* _topics;
	TopicIdMap _waitedPubTopicIdMap;
	TopicIdMap _waitedSubTopicIdMap;

	Connect _connectData;
	MQTTSNPacket* _connAck;

	char* _clientId;
	char* _willTopic;
	char* _willMsg;

	Timer _keepAliveTimer;
	uint32_t _keepAliveMsec;

	ClientStatus _status;
	bool _waitWillMsgFlg;

	uint16_t _packetId;
	uint8_t _snMsgId;

	Network* _network;      // Broker
	bool _secureNetwork;    // SSL
	bool _sensorNetype;     // false: unstable network like a G3
	SensorNetAddress _sensorNetAddr;

	Client* _nextClient;
	Client* _prevClient;

};

/*=====================================
 Class ClientList
 =====================================*/
class ClientList
{
public:
	ClientList();
	~ClientList();
	bool authorize(const char* fileName);
	void erase(Client*);
	Client* getClient(SensorNetAddress* addr);
	Client* createClient(SensorNetAddress* addr, MQTTSNString* clientId, bool unstableLine,
			bool secure);
	uint16_t getClientCount(void);
	Client* getClient(void);
	bool isAuthorized();
private:
	Client* _firstClient;
	Client* _endClient;
	Mutex _mutex;
	uint16_t _clientCnt;
	bool _authorize;
};

}
#endif /* MQTTSNGWCLIENT_H_ */

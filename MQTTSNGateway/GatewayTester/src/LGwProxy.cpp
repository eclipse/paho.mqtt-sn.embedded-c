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

#include <string.h>
#include <stdio.h>

#include "LMqttsnClientApp.h"
#include "LGwProxy.h"
#include "LMqttsnClient.h"
#include "LScreen.h"

using namespace std;
using namespace linuxAsyncClient;

extern void setUint16(uint8_t* pos, uint16_t val);
extern uint16_t getUint16(const uint8_t* pos);
extern LMqttsnClient* theClient;
extern LScreen* theScreen;
/*=====================================
        Class GwProxy
 ======================================*/
static const char* packet_names[] =
{
		"ADVERTISE", "SEARCHGW", "GWINFO", "RESERVED", "CONNECT", "CONNACK",
		"WILLTOPICREQ", "WILLTOPIC", "WILLMSGREQ", "WILLMSG", "REGISTER", "REGACK",
		"PUBLISH", "PUBACK", "PUBCOMP", "PUBREC", "PUBREL", "RESERVED",
		"SUBSCRIBE", "SUBACK", "UNSUBSCRIBE", "UNSUBACK", "PINGREQ", "PINGRESP",
		"DISCONNECT", "RESERVED", "WILLTOPICUPD", "WILLTOPICRESP", "WILLMSGUPD",
		"WILLMSGRESP"
};

LGwProxy::LGwProxy(){
	_nextMsgId = 0;
	_status = GW_LOST;
    _gwId = 0;
	_willTopic = 0;
	_willMsg = 0;
	_qosWill = 0;
	_retainWill = 0;
	_tkeepAlive = MQTTSN_DEFAULT_KEEPALIVE;
	_tAdv = MQTTSN_DEFAULT_DURATION;
	_cleanSession = 0;
	_pingStatus = 0;
	_connectRetry = MQTTSN_RETRY_COUNT;
}

LGwProxy::~LGwProxy(){
	_topicTbl.clearTopic();
}

void LGwProxy::initialize(LUdpConfig netconf, LMqttsnConfig mqconf){
	_network.initialize(netconf);
	_clientId = netconf.clientId;
    _willTopic = mqconf.willTopic;
    _willMsg = mqconf.willMsg;
    _qosWill = mqconf.willQos;
    _retainWill = mqconf.willRetain;
    _cleanSession = mqconf.cleanSession;
    _tkeepAlive = mqconf.keepAlive;
}

void LGwProxy::connect(){
	char* pos;

	while (_status != GW_CONNECTED){
		pos = _msg;

		if (_status == GW_SEND_WILLMSG){
			*pos++ = 2 + (uint8_t)strlen(_willMsg);
			*pos++ = MQTTSN_TYPE_WILLMSG;
			strcpy(pos,_willMsg);          // WILLMSG
			_status = GW_WAIT_CONNACK;
			writeGwMsg();
		}else if (_status == GW_SEND_WILLTOPIC){
			*pos++ = 3 + (uint8_t)strlen(_willTopic);
			*pos++ = MQTTSN_TYPE_WILLTOPIC;
			*pos++ = _qosWill | _retainWill;
			strcpy(pos,_willTopic);        // WILLTOPIC
			_status = GW_WAIT_WILLMSGREQ;
			writeGwMsg();
		}else if (_status == GW_CONNECTING || _status == GW_DISCONNECTED){
			uint8_t clientIdLen = uint8_t(strlen(_clientId) > 23 ? 23 : strlen(_clientId));
			*pos++ = 6 + clientIdLen;
			*pos++ = MQTTSN_TYPE_CONNECT;
			pos++;
			if (_cleanSession){
				_msg[2] = MQTTSN_FLAG_CLEAN;
			}
			*pos++ = MQTTSN_PROTOCOL_ID;
			setUint16((uint8_t*)pos, _tkeepAlive);
			pos += 2;
			strncpy(pos, _clientId, clientIdLen);
			_msg[ 6 + clientIdLen] = 0;
			if (_willMsg && _willTopic){
				if (strlen(_willMsg) && strlen(_willTopic)){
					_msg[2] = _msg[2] | MQTTSN_FLAG_WILL;   // CONNECT
					_status = GW_WAIT_WILLTOPICREQ;
				}
			}else{
				_status = GW_WAIT_CONNACK;
			}
			writeGwMsg();
			_connectRetry = MQTTSN_RETRY_COUNT;
		}else if (_status == GW_LOST){

			*pos++ = 3;
			*pos++ = MQTTSN_TYPE_SEARCHGW;
			*pos = 0;                        // SERCHGW
			_status = GW_SEARCHING;
			writeGwMsg();

		}
		getConnectResponce();
	}
	return;
}

int LGwProxy::getConnectResponce(void){
	int len = readMsg();

	if (len == 0){
		if (_sendUTC + MQTTSN_TIME_RETRY < LTimer::getUnixTime()){
			if (_msg[1] == MQTTSN_TYPE_CONNECT)
			{
				_connectRetry--;
			}
			if (--_retryCount > 0){
				writeMsg((const uint8_t*)_msg);  // Not writeGwMsg() : not to reset the counter.
				_sendUTC = LTimer::getUnixTime();
			}else{
				_sendUTC = 0;
				if ( _status > GW_SEARCHING && _connectRetry > 0){
					_status = GW_CONNECTING;
				}else{
					_status = GW_LOST;
					_gwId = 0;
				}
				return -1;
			}
		}
		return 0;
	}else if (_mqttsnMsg[0] == MQTTSN_TYPE_GWINFO && _status == GW_SEARCHING){
		_network.setGwAddress();
        _gwId = _mqttsnMsg[1];
		_status = GW_CONNECTING;
	}else if (_mqttsnMsg[0] == MQTTSN_TYPE_WILLTOPICREQ && _status == GW_WAIT_WILLTOPICREQ){
		_status = GW_SEND_WILLTOPIC;
	}else if (_mqttsnMsg[0] == MQTTSN_TYPE_WILLMSGREQ && _status == GW_WAIT_WILLMSGREQ){
		_status = GW_SEND_WILLMSG;
	}else if (_mqttsnMsg[0] == MQTTSN_TYPE_CONNACK && _status == GW_WAIT_CONNACK){
		if (_mqttsnMsg[1] == 0x00){
			_status = GW_CONNECTED;
			_connectRetry = MQTTSN_RETRY_COUNT;
			_keepAliveTimer.start(_tkeepAlive * 1000);
			_topicTbl.clearTopic();
			ASSERT("\033[0m\033[0;32m\n\n Connected to the BrokerMQTTSN-embedded-C\033[0m\033[0;37m\n\n");
			theClient->onConnect();  // SUBSCRIBEs are conducted
		}else{
			_status = GW_CONNECTING;
		}
	}
	return 1;
}

void LGwProxy::reconnect(void){
	D_MQTTLOG("...Gateway reconnect\r\n");
	_status = GW_DISCONNECTED;
	connect();
}

void LGwProxy::disconnect(uint16_t secs){
    _tSleep = secs;
    _status = GW_DISCONNECTING;
    
	_msg[1] = MQTTSN_TYPE_DISCONNECT;

	if (secs){
		_msg[0] = 4;
		setUint16((uint8_t*) _msg + 2, secs);
	}else{
		_msg[0] = 2;
		_keepAliveTimer.stop();
	}

	_retryCount = MQTTSN_RETRY_COUNT;
	writeMsg((const uint8_t*)_msg);
	_sendUTC = LTimer::getUnixTime();

	while ( _status != GW_DISCONNECTED && _status != GW_SLEPT){
		if (getDisconnectResponce() < 0){
			_status = GW_LOST;
			ASSERT("\033[0m\033[0;31m\n\n!!!!!! DISCONNECT  Error !!!!!\033[0m\033[0;37m \n\n");
			return;
		}
	}
}

int LGwProxy::getDisconnectResponce(void){
	int len = readMsg();

	if (len == 0){
		if (_sendUTC + MQTTSN_TIME_RETRY < LTimer::getUnixTime()){
			if (--_retryCount >= 0){
				writeMsg((const uint8_t*)_msg);
				_sendUTC = LTimer::getUnixTime();
			}else{
				_status = GW_LOST;
				_gwId = 0;
				return -1;
			}
		}
		return 0;
	}else if (_mqttsnMsg[0] == MQTTSN_TYPE_DISCONNECT){
		if (_tSleep){
			_status = GW_SLEEPING;
			_keepAliveTimer.start(_tSleep);
		}else{
			_status = GW_DISCONNECTED;
		}
	}
	return 0;
}

int LGwProxy::getMessage(void){
	int len = readMsg();
	if (len < 0){
		return len;   //error
	}
#ifdef DEBUG_MQTTSN
	if (len){
		D_MQTTLOG(" recved msgType %x\n", _mqttsnMsg[0]);
	}
#endif

	if (len == 0){
		// Check PINGREQ required
		checkPingReq();

		// Check ADVERTISE valid
		checkAdvertise();

		// Check Timeout of REGISTERs
		_regMgr.checkTimeout();

		// Check Timeout of PUBLISHes,
		theClient->getPublishManager()->checkTimeout();

		// Check Timeout of SUBSCRIBEs,
        theClient->getSubscribeManager()->checkTimeout();

	}else if (_mqttsnMsg[0] == MQTTSN_TYPE_PUBLISH){
        theClient->getPublishManager()->published(_mqttsnMsg, len);

    }else if (_mqttsnMsg[0] == MQTTSN_TYPE_PUBACK || _mqttsnMsg[0] == MQTTSN_TYPE_PUBCOMP ||
			_mqttsnMsg[0] == MQTTSN_TYPE_PUBREC || _mqttsnMsg[0] == MQTTSN_TYPE_PUBREL ){
        theClient->getPublishManager()->responce(_mqttsnMsg, (uint16_t)len);

    }else if (_mqttsnMsg[0] == MQTTSN_TYPE_SUBACK || _mqttsnMsg[0] == MQTTSN_TYPE_UNSUBACK){
        theClient->getSubscribeManager()->responce(_mqttsnMsg);

    }else if (_mqttsnMsg[0] == MQTTSN_TYPE_REGISTER){
    	_regMgr.responceRegister(_mqttsnMsg, len);

    }else if (_mqttsnMsg[0] == MQTTSN_TYPE_REGACK){
        _regMgr.responceRegAck(getUint16(_mqttsnMsg + 3), getUint16(_mqttsnMsg + 1));

    }else if (_mqttsnMsg[0] == MQTTSN_TYPE_PINGRESP){
        if (_pingStatus == GW_WAIT_PINGRESP){
            _pingStatus = 0;
            resetPingReqTimer();
        }
    }else if (_mqttsnMsg[0] == MQTTSN_TYPE_DISCONNECT){
    	_status = GW_LOST;
    	_gwAliveTimer.stop();
    	_keepAliveTimer.stop();
    }else if (_mqttsnMsg[0] == MQTTSN_TYPE_ADVERTISE){
    	if (getUint16((const uint8_t*)(_mqttsnMsg + 2)) < 61){
    		_tAdv = getUint16((const uint8_t*)(_mqttsnMsg + 2)) * 1500;
    	}else{
    		_tAdv = getUint16((const uint8_t*)(_mqttsnMsg + 2)) * 1100;
    	}
        _gwAliveTimer.start(_tAdv);
    }
    return 0;
}




uint16_t LGwProxy::registerTopic(char* topicName, uint16_t topicId){
	uint16_t id = topicId;
    if (id == 0){
        id = _topicTbl.getTopicId(topicName);
		_regMgr.registerTopic(topicName);
    }
	return id;
}


int LGwProxy::writeMsg(const uint8_t* msg){
	uint16_t len;
	uint8_t pos;
	uint8_t  rc;

	if (msg[0] == 0x01){
		len = getUint16(msg + 1);
		pos = 2;
	}else{
		len = msg[0];
		pos = 1;
	}

	if (msg[0] == 3 && msg[1] == MQTTSN_TYPE_SEARCHGW){
		rc = _network.broadcast(msg,len);
	}else{
		rc = _network.unicast(msg,len);
	}

	if (rc > 0){
		if ( msg[pos] >= MQTTSN_TYPE_ADVERTISE && msg[pos] <= MQTTSN_TYPE_WILLMSGRESP )
		{
			ASSERT("  send %s\n", packet_names[msg[pos]]);
		}
		return rc;
	}
	//_status = GW_LOST;
	//_gwId = 0;
	return rc;
}

void LGwProxy::writeGwMsg(void){
	_retryCount = MQTTSN_RETRY_COUNT;
	writeMsg((const uint8_t*)_msg);
	_sendUTC = LTimer::getUnixTime();
}

int LGwProxy::readMsg(void){
	int len = 0;
	_mqttsnMsg = _network.getMessage(&len);
	if (len == 0){
		return 0;
	}

	if (_mqttsnMsg[0] == 0x01){
		int msgLen = (int) getUint16((const uint8_t*)_mqttsnMsg + 1);
		if (len != msgLen){
			_mqttsnMsg += 3;
			len = msgLen - 3;
		}
	}else{
		_mqttsnMsg += 1;
		len -= 1;
	}
	if ( *_mqttsnMsg >= MQTTSN_TYPE_ADVERTISE && *_mqttsnMsg <= MQTTSN_TYPE_WILLMSGRESP )
	{
		ASSERT("  recv %s\n", packet_names[*_mqttsnMsg]);
	}
	return len;
}

void LGwProxy::setWillTopic(const char* willTopic, uint8_t qos, bool retain){
	_willTopic = willTopic;
	_retainWill = _qosWill = 0;
	if (qos == 1){
		_qosWill = MQTTSN_FLAG_QOS_1;
	}else if (qos == 2){
		_qosWill = MQTTSN_FLAG_QOS_2;
	}
	if (retain){
		_retainWill = MQTTSN_FLAG_RETAIN;
	}
}
void LGwProxy::setWillMsg(const char* willMsg){
	_willMsg = willMsg;
}


void LGwProxy::setCleanSession(bool flg){
	if (flg){
		_cleanSession = MQTTSN_FLAG_CLEAN;
	}else{
		_cleanSession = 0;
	}
}

uint16_t LGwProxy::getNextMsgId(void){
	_nextMsgId++;
    if (_nextMsgId == 0){
    	_nextMsgId = 1;
    }
    return _nextMsgId;
}

void LGwProxy::checkPingReq(void){
    uint8_t msg[2];
	msg[0] = 0x02;
	msg[1] = MQTTSN_TYPE_PINGREQ;
    
	if (_status == GW_CONNECTED && _keepAliveTimer.isTimeUp() && _pingStatus != GW_WAIT_PINGRESP){
		_pingStatus = GW_WAIT_PINGRESP;
        _pingRetryCount = MQTTSN_RETRY_COUNT;

		writeMsg((const uint8_t*)msg);
        _pingSendUTC = LTimer::getUnixTime();
	}else if (_pingStatus == GW_WAIT_PINGRESP){
        if (_pingSendUTC + MQTTSN_TIME_RETRY < LTimer::getUnixTime()){
    		if (--_pingRetryCount > 0){
				writeMsg((const uint8_t*)msg);
				_pingSendUTC = LTimer::getUnixTime();
			}else{
				_status = GW_LOST;
				_gwId = 0;
                _pingStatus = 0;
                _keepAliveTimer.stop();
                D_MQTTLOG("   !!! PINGREQ Timeout\n");
			}
		}
	}
}

void LGwProxy::checkAdvertise(void){
	if ( _gwAliveTimer.isTimeUp()){
		_status = GW_LOST;
		_gwId = 0;
		_pingStatus = 0;
		_gwAliveTimer.stop();
		_keepAliveTimer.stop();
		D_MQTTLOG("   !!! ADVERTISE Timeout\n");
	}
}

LTopicTable* LGwProxy::getTopicTable(void){
	return &_topicTbl;
}

LRegisterManager* LGwProxy::getRegisterManager(void){
	return &_regMgr;
}

void LGwProxy::resetPingReqTimer(void){
	_keepAliveTimer.start(_tkeepAlive * 1000);
}

const char* LGwProxy::getClientId(void) {
	return _clientId;
}

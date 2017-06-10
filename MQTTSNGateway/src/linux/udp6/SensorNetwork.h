/**************************************************************************************
 * Copyright (c) 2017, Benjamin Aigner
 * based on UDP implementation (Copyright (c) 2016 Tomoaki Yamaguchi)
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
 * 	  Benjamin Aigner - port to UDPv6, used by RFC7668 (6lowpan over Bluetooth LE)
 *    Tomoaki Yamaguchi - initial API and implementation and/or initial documentation
 **************************************************************************************/

#ifndef SENSORNETWORK_H_
#define SENSORNETWORK_H_

#include "MQTTSNGWDefines.h"
#include <arpa/inet.h>
#include <string>

using namespace std;

namespace MQTTSNGW
{

#ifdef  DEBUG_NWSTACK
  #define D_NWSTACK(...) printf(__VA_ARGS__)
#else
  #define D_NWSTACK(...)
#endif

/*===========================================
 Class  SensorNetAddreess
 ============================================*/
class SensorNetAddress
{
public:
	SensorNetAddress();
	~SensorNetAddress();
	void setAddress(struct sockaddr_in6 *IpAddr, uint16_t port);
	int  setAddress(string* data);
	int  setAddress(const char* data);
	uint16_t getPortNo(void);
	struct sockaddr_in6 *getIpAddress(void);
	char* getAddress(void);
	bool isMatch(SensorNetAddress* addr);
	SensorNetAddress& operator =(SensorNetAddress& addr);
	char* sprint(char* buf);
private:
	uint16_t _portNo;
	char _addrString[INET6_ADDRSTRLEN+1];
	struct sockaddr_in6 _IpAddr;
};

/*========================================
 Class UpdPort
 =======================================*/
class UDPPort6
{
public:
	UDPPort6();
	virtual ~UDPPort6();

	int open(const char* ipAddress, uint16_t uniPortNo, const char* broadcastAddr, const char* interfaceName);
	void close(void);
	int unicast(const uint8_t* buf, uint32_t length, SensorNetAddress* sendToAddr);
	int broadcast(const uint8_t* buf, uint32_t length);
	int recv(uint8_t* buf, uint16_t len, SensorNetAddress* addr);

private:
	void setNonBlocking(const bool);
	int recvfrom(int sockfd, uint8_t* buf, uint16_t len, uint8_t flags,	SensorNetAddress* addr);

	int _sockfdUnicast;
	int _sockfdMulticast;
	char _interfaceName[10];

	SensorNetAddress _grpAddr;
	SensorNetAddress _clientAddr;
	uint16_t _uniPortNo;
	bool _disconReq;

};

/*===========================================
 Class  SensorNetwork
 ============================================*/
class SensorNetwork: public UDPPort6
{
public:
	SensorNetwork();
	~SensorNetwork();

	int unicast(const uint8_t* payload, uint16_t payloadLength, SensorNetAddress* sendto);
	int broadcast(const uint8_t* payload, uint16_t payloadLength);
	int read(uint8_t* buf, uint16_t bufLen);
	int initialize(void);
	const char* getDescription(void);
	SensorNetAddress* getSenderAddress(void);

private:
	SensorNetAddress _clientAddr;   // Sender's address. not gateway's one.
	string _description;
};

}
#endif /* SENSORNETWORK6_H_ */

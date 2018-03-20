/**************************************************************************************
 * Copyright (c) 2017, Benjamin Aigner
 * Copyright (c) 2016, Tomoaki Yamaguchi (original UDPv4 implementation)
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
 *    Benjamin Aigner - Adaption of the UDPv4 code to use UDPv6
 *    Tomoaki Yamaguchi - initial API and implementation and/or initial documentation
 *    Tieto Poland Sp. z o.o. - improve portability
 **************************************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <iostream>
#include <regex>
#include <string>
#include <stdlib.h>
#include "SensorNetwork.h"
#include "MQTTSNGWProcess.h"

//using namespace std;
using namespace MQTTSNGW;

/*===========================================
 Class  SensorNetAddreess
 ============================================*/
SensorNetAddress::SensorNetAddress()
{
	_portNo = 0;
	memset((void *)&_IpAddr,0,sizeof(_IpAddr));
}

SensorNetAddress::~SensorNetAddress()
{
}

struct sockaddr_in6 *SensorNetAddress::getIpAddress(void)
{
	return &_IpAddr;
}

uint16_t SensorNetAddress::getPortNo(void)
{
	return _portNo;
}

void SensorNetAddress::setAddress(struct sockaddr_in6 *IpAddr, uint16_t port)
{
	memcpy((void *)&_IpAddr,IpAddr,sizeof(_IpAddr));
	_portNo = port;
}

/**
 *  convert Text data to SensorNetAddress
 *  @param  buf is pointer of IP_Address:PortNo format text
 *  @return success = 0,  Invalid format = -1
 */
int SensorNetAddress::setAddress(string* data)
{
	const char *cstr = data->c_str();
	inet_pton(AF_INET6, cstr, &(_IpAddr.sin6_addr));
	return 0;
}
/**
 *  convert Text data to SensorNetAddress
 *  @param  buf is pointer of IP_Address:PortNo format text
 *  @return success = 0,  Invalid format = -1
 */
int SensorNetAddress::setAddress(const char* data)
{
	inet_pton(AF_INET6, data, &(_IpAddr.sin6_addr));
	return 0;
}

char* SensorNetAddress::getAddress(void)
{
	inet_ntop(AF_INET6, &(_IpAddr.sin6_addr), _addrString, INET6_ADDRSTRLEN);
	return _addrString;
}

bool SensorNetAddress::isMatch(SensorNetAddress* addr)
{
	return ((this->_portNo == addr->_portNo) && \
	(this->_IpAddr.sin6_addr.s6_addr32[0] == addr->_IpAddr.sin6_addr.s6_addr32[0]) && \
	(this->_IpAddr.sin6_addr.s6_addr32[1] == addr->_IpAddr.sin6_addr.s6_addr32[1]) && \
	(this->_IpAddr.sin6_addr.s6_addr32[2] == addr->_IpAddr.sin6_addr.s6_addr32[2]) && \
	(this->_IpAddr.sin6_addr.s6_addr32[3] == addr->_IpAddr.sin6_addr.s6_addr32[3]));
}

SensorNetAddress& SensorNetAddress::operator =(SensorNetAddress& addr)
{
	this->_portNo = addr._portNo;
	memcpy(&this->_IpAddr.sin6_addr, &addr._IpAddr.sin6_addr, sizeof(this->_IpAddr.sin6_addr));
	return *this;
}


char* SensorNetAddress::sprint(char* buf)
{
	char ip[INET6_ADDRSTRLEN];
	inet_ntop(AF_INET6, &(_IpAddr.sin6_addr), ip, INET6_ADDRSTRLEN);
	sprintf( buf, "%s:", ip);
	sprintf( buf + strlen(buf), "%d", ntohs(_portNo));
	return buf;
}

/*===========================================
 Class  SensorNetwork
 ============================================*/
SensorNetwork::SensorNetwork()
{
}

SensorNetwork::~SensorNetwork()
{
}

int SensorNetwork::unicast(const uint8_t* payload, uint16_t payloadLength, SensorNetAddress* sendToAddr)
{
	return UDPPort6::unicast(payload, payloadLength, sendToAddr);
}

int SensorNetwork::broadcast(const uint8_t* payload, uint16_t payloadLength)
{
	return UDPPort6::broadcast(payload, payloadLength);
}

int SensorNetwork::read(uint8_t* buf, uint16_t bufLen)
{
	return UDPPort6::recv(buf, bufLen, &_clientAddr);
}

int SensorNetwork::initialize(void)
{
	char param[MQTTSNGW_PARAM_MAX];
	uint16_t unicastPortNo = 0;
	string ip;
	string broadcast;
	string interface;

	if (theProcess->getParam("GatewayUDP6Bind", param) == 0)
	{
		ip = param;
		_description = "GatewayUDP6Bind: ";
		_description += param;
	}
	if (theProcess->getParam("GatewayUDP6Port", param) == 0)
	{
		unicastPortNo = atoi(param);
		_description += " Gateway Port: ";
		_description += param;
	}
	if (theProcess->getParam("GatewayUDP6Broadcast", param) == 0)
	{
		broadcast = param;
		_description += " Broadcast Address: ";
		_description += param;
	}
	if (theProcess->getParam("GatewayUDP6If", param) == 0)
	{
		interface = param;
		_description += " Interface: ";
		_description += param;
	}

	return UDPPort6::open(ip.c_str(), unicastPortNo, broadcast.c_str(), interface.c_str());
}

const char* SensorNetwork::getDescription(void)
{
	return _description.c_str();
}

SensorNetAddress* SensorNetwork::getSenderAddress(void)
{
	return &_clientAddr;
}

/*=========================================
 Class udpStack
 =========================================*/

UDPPort6::UDPPort6()
{
	_disconReq = false;
	_sockfdUnicast = -1;
	_sockfdMulticast = -1;
}

UDPPort6::~UDPPort6()
{
	close();
}

void UDPPort6::close(void)
{
	if (_sockfdUnicast > 0)
	{
		::close(_sockfdUnicast);
		_sockfdUnicast = -1;
	}
	if (_sockfdMulticast > 0)
	{
		::close(_sockfdMulticast);
		_sockfdMulticast = -1;
	}
}

int UDPPort6::open(const char* ipAddress, uint16_t uniPortNo, const char* broadcastAddr, const char* interfaceName)
{
	struct addrinfo hints, *res;
	int errnu;
	const int reuse = 1;

	if (uniPortNo == 0)
	{
		WRITELOG("error portNo undefined in UDPPort::open\n");
		return -1;
	}


	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET6;  // use IPv6
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; //use local IF address

	getaddrinfo(NULL, std::to_string(uniPortNo).c_str(), &hints, &res);

	_sockfdMulticast = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if(_sockfdMulticast <0)
	{
		WRITELOG("UDP6::open - multicast: %s",strerror(_sockfdMulticast));
		return errno;
	}

	//select the interface
	unsigned int ifindex;
	ifindex = if_nametoindex(interfaceName);
	errnu = setsockopt(_sockfdMulticast, IPPROTO_IPV6, IPV6_MULTICAST_IF, &ifindex,sizeof(ifindex));
	if(errnu <0)
	{
		WRITELOG("UDP6::open - limit IF: %s",strerror(errnu));
		return errnu;
	}

	strcpy(_interfaceName,interfaceName);

	//restrict the socket to IPv6 only
	int on = 1;
	errnu = setsockopt(_sockfdMulticast, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&on, sizeof(on));
	if(errnu <0)
	{
		WRITELOG("UDP6::open - limit IPv6: %s",strerror(errnu));
		return errnu;
	}

	_uniPortNo = uniPortNo;
	freeaddrinfo(res);

	//init the structs for getaddrinfo
	//according to: https://beej.us/guide/bgnet/output/html/multipage/
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET6;  // use IPv6, whichever
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

	//no specific address, bind to available ones...
	getaddrinfo(NULL, std::to_string(uniPortNo).c_str(), &hints, &res);

	//create the socket
	_sockfdUnicast = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (_sockfdUnicast < 0)
	{
		WRITELOG("UDP6::open - unicast socket: %s",strerror(_sockfdUnicast));
		return -1;
	}

	//if given, set a given device name to bind to
	if(strlen(interfaceName) > 0)
	{
		//socket option: bind to a given interface name
		setsockopt(_sockfdUnicast, SOL_SOCKET, SO_BINDTODEVICE, interfaceName, strlen(interfaceName));
	}

	//socket option: reuse address
	setsockopt(_sockfdUnicast, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

	//finally: bind...
	errnu = ::bind(_sockfdUnicast, res->ai_addr, res->ai_addrlen);
	if (errnu  < 0)
	{
		WRITELOG("error can't bind unicast socket in UDPPort::open: %s\n",strerror(errnu));
		return -1;
	}

	//if given, set a broadcast address; otherwise it will be ::
	if(strlen(broadcastAddr) > 0)
	{
		_grpAddr.setAddress(broadcastAddr);
	} else {
		_grpAddr.setAddress("::");
	}
	//everything went fine...
	freeaddrinfo(res);
	return 0;
}

//TODO: test if unicast is working too....
int UDPPort6::unicast(const uint8_t* buf, uint32_t length, SensorNetAddress* addr)
{
	char destStr[INET6_ADDRSTRLEN+10];
	struct addrinfo hints, *res;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET6;  // use IPv6
	hints.ai_socktype = SOCK_DGRAM;

	int port = 0;
	string portStr;
	if(addr->getPortNo() != 0)
	{
		port = htons(addr->getPortNo());
		portStr = to_string(port);
	} else {
		port = _uniPortNo;
		portStr = to_string(port);
	}

	if(strlen(_interfaceName) != 0)
	{
		strcpy(destStr, addr->getAddress());
		strcat(destStr,"%");
		strcat(destStr,_interfaceName);
		if(IN6_IS_ADDR_LINKLOCAL(addr->getAddress()))
		{
			getaddrinfo(destStr, portStr.c_str(), &hints, &res);
		}
		else
		{
			getaddrinfo(addr->getAddress(), portStr.c_str(), &hints, &res);
		}
	} else {
		strcpy(destStr, addr->getAddress());
		getaddrinfo(addr->getAddress(), portStr.c_str(), &hints, &res);
	}

	int status = ::sendto(_sockfdUnicast, buf, length, 0, res->ai_addr, res->ai_addrlen);

	if (status < 0)
	{
		WRITELOG("errno in UDPPort::unicast(sendto): %d, %s\n",status,strerror(status));
	}

	WRITELOG("unicast sendto %s, port: %d length = %d\n", destStr,port,status);

	return status;
}

int UDPPort6::broadcast(const uint8_t* buf, uint32_t length)
{
	struct addrinfo hint,*info;
	int err;
	memset( &hint, 0, sizeof( hint ) );

	hint.ai_family = AF_INET6;
	hint.ai_socktype = SOCK_DGRAM;
	hint.ai_protocol = 0;



	if(strlen(_interfaceName) != 0)
	{
		char destStr[80];
		strcpy(destStr, _grpAddr.getAddress());
		strcat(destStr,"%");
		strcat(destStr,_interfaceName);
		if(IN6_IS_ADDR_MC_NODELOCAL(_grpAddr.getAddress()) ||
		   IN6_IS_ADDR_MC_LINKLOCAL(_grpAddr.getAddress()))
		{
			err = getaddrinfo(destStr, std::to_string(_uniPortNo).c_str(), &hint, &info );
		}
		else
		{
			err = getaddrinfo(_grpAddr.getAddress(), std::to_string(_uniPortNo).c_str(), &hint, &info );
		}
	} else {
		err = getaddrinfo(_grpAddr.getAddress(), std::to_string(_uniPortNo).c_str(), &hint, &info );
	}

	if( err != 0 ) {
	    WRITELOG("UDP6::broadcast - getaddrinfo: %s",strerror(err));
	    return err;
	}

	err = sendto(_sockfdMulticast, buf, length, 0, info->ai_addr, info->ai_addrlen );

	if(err < 0 ) {
	    WRITELOG("UDP6::broadcast - sendto: %s",strerror(err));
	    return errno;
	}

	return 0;
}

//TODO: test if this is working properly (GW works, but this function is not completely tested)
int UDPPort6::recv(uint8_t* buf, uint16_t len, SensorNetAddress* addr)
{
	struct timeval timeout;
	fd_set recvfds;

	timeout.tv_sec = 0;
	timeout.tv_usec = 1000000;    // 1 sec
	FD_ZERO(&recvfds);
	FD_SET(_sockfdUnicast, &recvfds);

	int rc = 0;
	if ( select(_sockfdUnicast + 1, &recvfds, 0, 0, &timeout) > 0 )
	{
		if (FD_ISSET(_sockfdUnicast, &recvfds))
		{
			rc = recvfrom(_sockfdUnicast, buf, len, 0, addr);
		}
	}
	return rc;
}

//TODO: test if this is working properly (GW works, but this function is not completely tested)
int UDPPort6::recvfrom(int sockfd, uint8_t* buf, uint16_t len, uint8_t flags, SensorNetAddress* addr)
{
	sockaddr_in6 sender;
	socklen_t addrlen = sizeof(sender);
	memset(&sender, 0, addrlen);

	int status = ::recvfrom(sockfd, buf, len, flags, (sockaddr*) &sender, &addrlen);

	if (status < 0 && errno != EAGAIN)
	{
		WRITELOG("errno == %d in UDPPort::recvfrom: %s\n",errno,strerror(errno));
		return -1;
	}
	addr->setAddress(&sender, (uint16_t)sender.sin6_port);
	//D_NWSTACK("recved from %s:%d length = %d\n", inet_ntoa(sender.sin_addr),ntohs(sender.sin_port), status);
	return status;
}

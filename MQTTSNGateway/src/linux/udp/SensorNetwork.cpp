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
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <regex>
#include <string>
#include <stdlib.h>
#include "SensorNetwork.h"
#include "MQTTSNGWProcess.h"

using namespace std;
using namespace MQTTSNGW;

/*===========================================
  Class  SensorNetAddreess

  These 4 methods are minimum requirements for the SensorNetAddress class.
   isMatch(SensorNetAddress* )
   operator =(SensorNetAddress& )
   setAddress(string* )
   sprint(char* )

  UDPPort class requires these 3 methods.
   getIpAddress(void)
   getPortNo(void)
   setAddress(uint32_t IpAddr, uint16_t port)

 ============================================*/
SensorNetAddress::SensorNetAddress()
{
	_portNo = 0;
	_IpAddr = 0;
}

SensorNetAddress::~SensorNetAddress()
{

}

uint32_t SensorNetAddress::getIpAddress(void)
{
	return _IpAddr;
}

uint16_t SensorNetAddress::getPortNo(void)
{
	return _portNo;
}

void SensorNetAddress::setAddress(uint32_t IpAddr, uint16_t port)
{
	_IpAddr = IpAddr;
	_portNo = port;
}

/**
 *  Set Address data to SensorNetAddress
 *
 *  @param  *ip_port is "IP_Address:PortNo" format string
 *  @return success = 0,  Invalid format = -1
 *
 *  This function is used in ClientList::authorize(const char* fileName)
 *  e.g.
 *  Authorized clients are defined by fileName = "clients.conf"
 *
 *  Client02,172.16.1.7:12002
 *  Client03,172.16.1.8:13003
 *  Client01,172.16.1.6:12001
 *
 *  This definition is necessary when using TLS connection.
 *  Gateway rejects clients not on the list for security reasons.
 *
 */
int SensorNetAddress::setAddress(string* ip_port)
{
	size_t pos = ip_port->find_first_of(":");

	if ( pos == string::npos )
	{
		_portNo = 0;
		_IpAddr = INADDR_NONE;
		return -1;
	}

	string ip = ip_port->substr(0, pos);
	string port = ip_port->substr(pos + 1);
	int portNo = 0;

	if ((portNo = atoi(port.c_str())) == 0 || (_IpAddr = inet_addr(ip.c_str())) == INADDR_NONE)
	{
		return -1;
	}
	_portNo = htons(portNo);
	return 0;
}

bool SensorNetAddress::isMatch(SensorNetAddress* addr)
{
	return ((this->_portNo == addr->_portNo) && (this->_IpAddr == addr->_IpAddr));
}

SensorNetAddress& SensorNetAddress::operator =(SensorNetAddress& addr)
{
	this->_portNo = addr._portNo;
	this->_IpAddr = addr._IpAddr;
	return *this;
}


char* SensorNetAddress::sprint(char* buf)
{
	struct in_addr  inaddr = { _IpAddr };
	char* ip = inet_ntoa(inaddr);
	sprintf( buf, "%s:", ip);
	sprintf( buf + strlen(buf), "%d", ntohs(_portNo));
	return buf;
}


/*================================================================
   Class  SensorNetwork

   In Gateway version 1.0

   getDescpription( )  is used by Gateway::initialize( )
   initialize( )       is used by ClientSendTask::initialize( )
   getSenderAddress( ) is used by ClientRecvTask::run( )
   broadcast( )        is used by MQTTSNPacket::broadcast( )
   unicast( )          is used by MQTTSNPacket::unicast( )
   read( )             is used by MQTTSNPacket::recv( )

 ================================================================*/

SensorNetwork::SensorNetwork()
{
}

SensorNetwork::~SensorNetwork()
{
}

int SensorNetwork::unicast(const uint8_t* payload, uint16_t payloadLength, SensorNetAddress* sendToAddr)
{
	return UDPPort::unicast(payload, payloadLength, sendToAddr);
}

int SensorNetwork::broadcast(const uint8_t* payload, uint16_t payloadLength)
{
	return UDPPort::broadcast(payload, payloadLength);
}

int SensorNetwork::read(uint8_t* buf, uint16_t bufLen)
{
	return UDPPort::recv(buf, bufLen, &_clientAddr);
}

/**
 *  Prepare UDP sockets and description of SensorNetwork like
 *   "UDP Multicast 225.1.1.1:1883 Gateway Port 10000".
 *   The description is for a start up prompt.
 *  @return success = 0, error = -1
 */
int SensorNetwork::initialize(void)
{
	char param[MQTTSNGW_PARAM_MAX];
	uint16_t multicastPortNo = 0;
	uint16_t unicastPortNo = 0;
	string ip;

	/*
	 * theProcess->getParam( ) copies
	 * a text specified by "Key" into param[] from the Gateway.conf
	 *
	 *  in Gateway.conf e.g.
	 *
	 *  # UDP
     *  GatewayPortNo=10000
     *  MulticastIP=225.1.1.1
     *  MulticastPortNo=1883
     *
	 */
	if (theProcess->getParam("MulticastIP", param) == 0)
	{
		ip = param;
		_description = "UDP Multicast ";
		_description += param;
	}
	if (theProcess->getParam("MulticastPortNo", param) == 0)
	{
		multicastPortNo = atoi(param);
		_description += ":";
		_description += param;
	}
	if (theProcess->getParam("GatewayPortNo", param) == 0)
	{
		unicastPortNo = atoi(param);
		_description += " Gateway Port ";
		_description += param;
	}

	/*  Prepare UDP sockets */
	return UDPPort::open(ip.c_str(), multicastPortNo, unicastPortNo);
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

UDPPort::UDPPort()
{
	_disconReq = false;
	_sockfdUnicast = -1;
	_sockfdMulticast = -1;
}

UDPPort::~UDPPort()
{
	close();
}

void UDPPort::close(void)
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

int UDPPort::open(const char* ipAddress, uint16_t multiPortNo, uint16_t uniPortNo)
{
	char loopch = 0;
	const int reuse = 1;

	if (uniPortNo == 0 || multiPortNo == 0)
	{
		D_NWSTACK("error portNo undefined in UDPPort::open\n");
		return -1;
	}

	uint32_t ip = inet_addr(ipAddress);
	_grpAddr.setAddress(ip, htons(multiPortNo));
	_clientAddr.setAddress(ip, htons(uniPortNo));

	/*------ Create unicast socket --------*/
	_sockfdUnicast = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (_sockfdUnicast < 0)
	{
		D_NWSTACK("error can't create unicast socket in UDPPort::open\n");
		return -1;
	}

	setsockopt(_sockfdUnicast, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

	sockaddr_in addru;
	addru.sin_family = AF_INET;
	addru.sin_port = htons(uniPortNo);
	addru.sin_addr.s_addr = INADDR_ANY;

	if (::bind(_sockfdUnicast, (sockaddr*) &addru, sizeof(addru)) < 0)
	{
		D_NWSTACK("error can't bind unicast socket in UDPPort::open\n");
		return -1;
	}
	if (setsockopt(_sockfdUnicast, IPPROTO_IP, IP_MULTICAST_LOOP, (char*) &loopch, sizeof(loopch)) < 0)
	{
		D_NWSTACK("error IP_MULTICAST_LOOP in UDPPort::open\n");
		close();
		return -1;
	}

	/*------ Create Multicast socket --------*/
	_sockfdMulticast = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (_sockfdMulticast < 0)
	{
		D_NWSTACK("error can't create multicast socket in UDPPort::open\n");
		close();
		return -1;
	}

	setsockopt(_sockfdMulticast, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

	sockaddr_in addrm;
	addrm.sin_family = AF_INET;
	addrm.sin_port = _grpAddr.getPortNo();
	addrm.sin_addr.s_addr = INADDR_ANY;

	if (::bind(_sockfdMulticast, (sockaddr*) &addrm, sizeof(addrm)) < 0)
	{
		D_NWSTACK("error can't bind multicast socket in UDPPort::open\n");
		return -1;
	}
	if (setsockopt(_sockfdMulticast, IPPROTO_IP, IP_MULTICAST_LOOP, (char*) &loopch, sizeof(loopch)) < 0)
	{
		D_NWSTACK("error IP_MULTICAST_LOOP in UDPPort::open\n");
		close();
		return -1;
	}

	ip_mreq mreq;
	mreq.imr_interface.s_addr = INADDR_ANY;
	mreq.imr_multiaddr.s_addr = _grpAddr.getIpAddress();

	if (setsockopt(_sockfdMulticast, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
	{
		D_NWSTACK("error Multicast IP_ADD_MEMBERSHIP in UDPPort::open\n");
		close();
		return -1;
	}

	if (setsockopt(_sockfdUnicast, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
	{
		D_NWSTACK("error Unicast IP_ADD_MEMBERSHIP in UDPPort::open\n");
		close();
		return -1;
	}
	return 0;
}

int UDPPort::unicast(const uint8_t* buf, uint32_t length, SensorNetAddress* addr)
{
	sockaddr_in dest;
	dest.sin_family = AF_INET;
	dest.sin_port = addr->getPortNo();
	dest.sin_addr.s_addr = addr->getIpAddress();

	int status = ::sendto(_sockfdUnicast, buf, length, 0, (const sockaddr*) &dest, sizeof(dest));
	if (status < 0)
	{
		D_NWSTACK("errno == %d in UDPPort::sendto\n", errno);
	}
	D_NWSTACK("sendto %s:%u length = %d\n", inet_ntoa(dest.sin_addr), ntohs(dest.sin_port), status);
	return status;
}

int UDPPort::broadcast(const uint8_t* buf, uint32_t length)
{
	return unicast(buf, length, &_grpAddr);
}

int UDPPort::recv(uint8_t* buf, uint16_t len, SensorNetAddress* addr)
{
	struct timeval timeout;
	fd_set recvfds;
	int maxSock = 0;

	timeout.tv_sec = 0;
	timeout.tv_usec = 1000000;    // 1 sec
	FD_ZERO(&recvfds);
	FD_SET(_sockfdUnicast, &recvfds);
	FD_SET(_sockfdMulticast, &recvfds);

	if (_sockfdMulticast > _sockfdUnicast)
	{
		maxSock = _sockfdMulticast;
	}
	else
	{
		maxSock = _sockfdUnicast;
	}

	int rc = 0;
	if ( select(maxSock + 1, &recvfds, 0, 0, &timeout) > 0 )
	{
		if (FD_ISSET(_sockfdUnicast, &recvfds))
		{
			rc = recvfrom(_sockfdUnicast, buf, len, 0, addr);
		}
		else if (FD_ISSET(_sockfdMulticast, &recvfds))
		{
			rc = recvfrom(_sockfdMulticast, buf, len, 0, &_grpAddr);
		}
	}
	return rc;
}

int UDPPort::recvfrom(int sockfd, uint8_t* buf, uint16_t len, uint8_t flags, SensorNetAddress* addr)
{
	sockaddr_in sender;
	socklen_t addrlen = sizeof(sender);
	memset(&sender, 0, addrlen);

	int status = ::recvfrom(sockfd, buf, len, flags, (sockaddr*) &sender, &addrlen);

	if (status < 0 && errno != EAGAIN)
	{
		D_NWSTACK("errno == %d in UDPPort::recvfrom\n", errno);
		return -1;
	}
	addr->setAddress(sender.sin_addr.s_addr, sender.sin_port);
	D_NWSTACK("recved from %s:%d length = %d\n", inet_ntoa(sender.sin_addr),ntohs(sender.sin_port), status);
	return status;
}


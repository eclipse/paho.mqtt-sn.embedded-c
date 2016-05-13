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
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <error.h>

#include "Network.h"
#include "MQTTSNGWDefines.h"
#include "MQTTSNGWProcess.h"

using namespace std;
using namespace MQTTSNGW;

#define SOCKET_MAXCONNECTIONS  5
char* currentDateTime();

/*========================================
 Class TCPStack
 =======================================*/
TCPStack::TCPStack()
{
	_addrinfo = 0;
	_disconReq = false;
	_sockfd = -1;
}

TCPStack::~TCPStack()
{
	if (_addrinfo)
	{
		freeaddrinfo(_addrinfo);
	}
}

bool TCPStack::isValid()
{
	if (_sockfd > 0)
	{
		if (_disconReq)
		{
			close();
			_sem.post();
		}
		else
		{
			return true;
		}
	}
	return false;
}

void TCPStack::disconnect()
{
	if (_sockfd > 0)
	{
		_disconReq = true;
		_sem.wait();
	}
}

void TCPStack::close()
{
	if (_sockfd > 0)
	{
		::close(_sockfd);
		_sockfd = -1;
		_disconReq = false;
		if (_addrinfo)
		{
			freeaddrinfo(_addrinfo);
			_addrinfo = 0;
		}
	}
}

bool TCPStack::bind(const char* service)
{
	if (isValid())
	{
		return false;
	}
	addrinfo hints;
	memset(&hints, 0, sizeof(addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if (_addrinfo)
	{
		freeaddrinfo(_addrinfo);
	}
	int err = getaddrinfo(0, service, &hints, &_addrinfo);
	if (err)
	{
		WRITELOG("\n%s   \x1b[0m\x1b[31merror:\x1b[0m\x1b[37mgetaddrinfo(): %s\n", currentDateTime(),
				gai_strerror(err));
		return false;
	}

	_sockfd = socket(_addrinfo->ai_family, _addrinfo->ai_socktype, _addrinfo->ai_protocol);
	if (_sockfd < 0)
	{
		return false;
	}
	int on = 1;
	if (setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*) &on, sizeof(on)) == -1)
	{
		return false;
	}

	if (::bind(_sockfd, _addrinfo->ai_addr, _addrinfo->ai_addrlen) < 0)
	{
		return false;
	}
	return true;
}

bool TCPStack::listen()
{
	if (!isValid())
	{
		return false;
	}
	int listen_return = ::listen(_sockfd, SOCKET_MAXCONNECTIONS);
	if (listen_return == -1)
	{
		return false;
	}
	return true;
}

bool TCPStack::accept(TCPStack& new_socket)
{
	sockaddr_storage sa;
	socklen_t len = sizeof(sa);
	new_socket._sockfd = ::accept(_sockfd, (struct sockaddr*) &sa, &len);
	if (new_socket._sockfd <= 0)
	{
		return false;
	}
	else
	{
		return true;
	}
}

int TCPStack::send(const uint8_t* buf, int length)
{
	return ::send(_sockfd, buf, length, MSG_NOSIGNAL);
}

int TCPStack::recv(uint8_t* buf, int len)
{
	return ::recv(_sockfd, buf, len, 0);
}

bool TCPStack::connect(const char* host, const char* service)
{
	if (isValid())
	{
		return false;
	}
	addrinfo hints;
	memset(&hints, 0, sizeof(addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	if (_addrinfo)
	{
		freeaddrinfo(_addrinfo);
	}

	int err = getaddrinfo(host, service, &hints, &_addrinfo);
	if (err)
	{
		WRITELOG("\n%s   \x1b[0m\x1b[31merror:\x1b[0m\x1b[37mgetaddrinfo(): %s\n", currentDateTime(),
				gai_strerror(err));
		return false;
	}

	int sockfd = socket(_addrinfo->ai_family, _addrinfo->ai_socktype, _addrinfo->ai_protocol);

	if (sockfd < 0)
	{
		return false;
	}
	int on = 1;

	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*) &on, sizeof(on)) == -1)
	{
		return false;
	}

	if (::connect(sockfd, _addrinfo->ai_addr, _addrinfo->ai_addrlen) < 0)
	{
		//perror("TCPStack connect");
		::close(sockfd);
		return false;
	}

	_sockfd = sockfd;
	return true;
}

void TCPStack::setNonBlocking(const bool b)
{
	int opts;

	opts = fcntl(_sockfd, F_GETFL);

	if (opts < 0)
	{
		return;
	}

	if (b)
	{
		opts = (opts | O_NONBLOCK);
	}
	else
	{
		opts = (opts & ~O_NONBLOCK);
	}
	fcntl(_sockfd, F_SETFL, opts);
}

int TCPStack::getSock()
{
	return _sockfd;
}

/*========================================
 Class Network
 =======================================*/
int Network::_numOfInstance = 0;
SSL_CTX* Network::_ctx = 0;
SSL_SESSION* Network::_session = 0;

Network::Network(bool secure) :
		TCPStack()
{
	char error[256];
	if (secure)
	{
		_numOfInstance++;
		if (_ctx == 0)
		{
			SSL_load_error_strings();
			SSL_library_init();
			_ctx = SSL_CTX_new(TLSv1_2_client_method());
			if (_ctx == 0)
			{
				ERR_error_string_n(ERR_get_error(), error, sizeof(error));
				WRITELOG("SSL_CTX_new() %s\n", error);
				throw Exception( ERR_get_error(), "Network can't create SSL context.");
			}
			if (!SSL_CTX_load_verify_locations(_ctx, 0, MQTTSNGW_TLS_CA_DIR))
			{
				ERR_error_string_n(ERR_get_error(), error, sizeof(error));
				WRITELOG("SSL_CTX_load_verify_locations() %s\n", error);
				throw Exception( ERR_get_error(), "Network can't load CA_LIST.");
			}
		}
	}
	_ssl = 0;
	_disconReq = false;
	_secureFlg = secure;
	_busy = false;
}

Network::~Network()
{
	if (_secureFlg)
	{
		_numOfInstance--;
	}
	if (_ssl)
	{
		SSL_free(_ssl);
	}
	if (_session && _numOfInstance == 0)
	{
		SSL_SESSION_free(_session);
		_session = 0;
	}
	if (_ctx && _numOfInstance == 0)
	{
		SSL_CTX_free(_ctx);
		_ctx = 0;
		ERR_free_strings();
	}
}

bool Network::connect(const char* host, const char* service)
{
	char errmsg[256];
	int rc = 0;
	char peer_CN[256];
	SSL_SESSION* sess = 0;
	X509* peer;

	if (isValid())
	{
		return false;
	}
	if (!TCPStack::connect(host, service))
	{
		return false;
	}
	if (!_secureFlg)
	{
		return true;
	}

	SSL* ssl = SSL_new(_ctx);
	if (ssl == 0)
	{
		ERR_error_string_n(ERR_get_error(), errmsg, sizeof(errmsg));
		WRITELOG("SSL_new()  %s\n", errmsg);
		return false;
	}

	rc = SSL_set_fd(ssl, TCPStack::getSock());
	if (rc == 0)
	{
		SSL_free(ssl);
		return false;
	}

	if (_session)
	{
		rc = SSL_set_session(ssl, sess);
	}
	else
	{
		rc = SSL_connect(ssl);
	}
	if (rc != 1)
	{
		ERR_error_string_n(ERR_get_error(), errmsg, sizeof(errmsg));
		WRITELOG("SSL_connect() %s\n", errmsg);
		SSL_free(ssl);
		return false;
	}

	if (SSL_get_verify_result(ssl) != X509_V_OK)
	{
		WRITELOG("SSL_get_verify_result() error: Certificate doesn't verify.\n");
		SSL_free(ssl);
		return false;
	}

	peer = SSL_get_peer_certificate(ssl);
	X509_NAME_get_text_by_NID(X509_get_subject_name(peer), NID_commonName, peer_CN, 256);
	if (strcasecmp(peer_CN, host))
	{
		WRITELOG("SSL_get_peer_certificate() error: Broker dosen't much host name.\n");
		SSL_free(ssl);
		return false;
	}
	if (_session == 0)
	{
		_session = sess;
	}
	_ssl = ssl;
	return true;
}

int Network::send(const uint8_t* buf, uint16_t length)
{
	char errmsg[256];
	fd_set rset;
	fd_set wset;
	bool writeBlockedOnRead = false;
	int bpos = 0;

	if (_secureFlg)
	{
		_mutex.lock();
		_busy = true;

		while (true)
		{
			FD_ZERO(&rset);
			FD_ZERO(&wset);
			FD_SET(getSock(), &rset);
			FD_SET(getSock(), &wset);

			int activity = select(getSock() + 1, &rset, &wset, 0, 0);
			if (activity > 0)
			{
				if (FD_ISSET(getSock(), &wset) || (writeBlockedOnRead && FD_ISSET(getSock(), &rset)))
				{

					writeBlockedOnRead = false;
					int r = SSL_write(_ssl, buf + bpos, length);

					switch (SSL_get_error(_ssl, r))
					{
					case SSL_ERROR_NONE:
						length -= r;
						bpos += r;
						if (length == 0)
						{
							_busy = false;
							_mutex.unlock();
							return bpos;
						}
						break;
					case SSL_ERROR_WANT_WRITE:
						break;
					case SSL_ERROR_WANT_READ:
						writeBlockedOnRead = true;
						break;
					default:
						ERR_error_string_n(ERR_get_error(), errmsg, sizeof(errmsg));
						WRITELOG("TLSStack::send() default %s\n", errmsg);
						_busy = false;
						_mutex.unlock();
						return -1;
					}
				}
			}
		}
	}
	else
	{
		return TCPStack::send(buf, length);
	}
}

int Network::recv(uint8_t* buf, uint16_t len)
{
	char errmsg[256];
	bool writeBlockedOnRead = false;
	bool readBlockedOnWrite = false;
	bool readBlocked = false;
	int rlen = 0;
	int bpos = 0;
	fd_set rset;
	fd_set wset;

	if (_secureFlg)
	{
		if (_busy)
		{
			return 0;
		}
		_mutex.lock();
		_busy = true;

		loop: do
		{
			readBlockedOnWrite = false;
			readBlocked = false;

			rlen = SSL_read(_ssl, buf + bpos, len - bpos);

			switch (SSL_get_error(_ssl, rlen))
			{
			case SSL_ERROR_NONE:
				_busy = false;
				_mutex.unlock();
				return rlen + bpos;
				break;
			case SSL_ERROR_ZERO_RETURN:
				SSL_shutdown(_ssl);
				_ssl = 0;
				TCPStack::close();
				_busy = false;
				_mutex.unlock();
				return -1;
				break;
			case SSL_ERROR_WANT_READ:
				readBlocked = true;
				break;
			case SSL_ERROR_WANT_WRITE:
				readBlockedOnWrite = true;
				break;
			default:
				ERR_error_string_n(ERR_get_error(), errmsg, sizeof(errmsg));
				WRITELOG("TLSStack::recv() default %s\n", errmsg);
				_busy = false;
				_mutex.unlock();
				return -1;
			}
		} while (SSL_pending(_ssl) && !readBlocked);

		bpos += rlen;
		while (true)
		{
			FD_ZERO(&rset);
			FD_ZERO(&wset);
			FD_SET(getSock(), &rset);
			FD_SET(getSock(), &wset);

			int activity = select(getSock() + 1, &rset, &wset, 0, 0);
			if (activity > 0)
			{
				if ((FD_ISSET(getSock(),&rset) && !writeBlockedOnRead)
						|| (readBlockedOnWrite && FD_ISSET(getSock(), &wset)))
				{
					goto loop;
				}
			}
			else
			{
				ERR_error_string_n(ERR_get_error(), errmsg, sizeof(errmsg));
				WRITELOG("TLSStack::recv() select %s\n", errmsg);
				_busy = false;
				_mutex.unlock();
				return -1;
			}
		}
	}
	return TCPStack::recv(buf, len);
}

bool Network::isValid()
{
	if (!_secureFlg)
	{
		return TCPStack::isValid();
	}
	if (_ssl)
	{
		if (_disconReq)
		{
			SSL_shutdown(_ssl);
			_ssl = 0;
			TCPStack::close();
			_disconReq = false;
		}
		else
		{
			return true;
		}
	}
	return false;
}

void Network::disconnect()
{
	if (_ssl)
	{
		_disconReq = true;
		TCPStack::disconnect();
	}
	else
	{
		TCPStack::disconnect();
	}
}

int Network::getSock()
{
	return TCPStack::getSock();
}

SSL* Network::getSSL()
{
	if (_secureFlg)
	{
		return _ssl;
	}
	else
	{
		return 0;
	}
}

bool Network::isSecure()
{
	return _secureFlg;
}


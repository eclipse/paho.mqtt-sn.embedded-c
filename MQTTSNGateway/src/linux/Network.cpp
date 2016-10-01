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
#include <regex>

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
	_sockfd = 0;
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
	return (_sockfd > 0);
}

void TCPStack::close()
{
	_mutex.lock();
	if (_sockfd > 0)
	{
		::close(_sockfd);
		_sockfd = 0;
		if (_addrinfo)
		{
			freeaddrinfo(_addrinfo);
			_addrinfo = 0;
		}
	}
	_mutex.unlock();

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

Network::Network(bool secure) :
		TCPStack()
{
	_ssl = 0;
	_secureFlg = secure;
	_busy = false;
	_session = 0;
	_sslValid = false;
}

Network::~Network()
{
	if (_ssl)
	{
		SSL_free(_ssl);
		_numOfInstance--;
	}
	if (_session )
	{
		SSL_SESSION_free(_session);
	}
	if (_ctx && _numOfInstance == 0)
	{
		SSL_CTX_free(_ctx);
		_ctx = 0;
		ERR_free_strings();
	}
}

bool Network::connect(const char* host, const char* port)
{
	if (_secureFlg)
	{
		return false;
	}

	if (getSock() == 0)
	{
		if (!TCPStack::connect(host, port))
		{
			return false;
		}
	}
	return true;
}

bool Network::connect(const char* host, const char* port, const char* caPath, const char* caFile, const char* cert, const char* prvkey)
{
	char errmsg[256];
	char peer_CN[256];
	int rc = 0;

	if (!_secureFlg)
	{
		WRITELOG("TLS is not required.\n");
		return false;
	}

	if (_ctx == 0)
	{
		SSL_load_error_strings();
		SSL_library_init();
		_ctx = SSL_CTX_new(TLS_client_method());
		if (_ctx == 0)
		{
			ERR_error_string_n(ERR_get_error(), errmsg, sizeof(errmsg));
			WRITELOG("SSL_CTX_new() %s\n", errmsg);
			return false;
		}

		if (!SSL_CTX_load_verify_locations(_ctx, caFile, caPath))
		{
			ERR_error_string_n(ERR_get_error(), errmsg, sizeof(errmsg));
			WRITELOG("SSL_CTX_load_verify_locations() %s\n", errmsg);
			return false;
		}
	}

	if (!_sslValid)
	{
		if ( !TCPStack::connect(host, port) )
		{
			return false;
		}
		/*
		if ( _ssl )
		{
			if (!SSL_set_fd(_ssl, getSock()))
			{
				ERR_error_string_n(ERR_get_error(), errmsg, sizeof(errmsg));
				WRITELOG("SSL_set_fd()  %s\n", errmsg);
				SSL_free(_ssl);
			}
			else
			{
				_sslValid = true;
				return true;
			}
		}
		*/
	}

	_ssl = SSL_new(_ctx);
	if (_ssl == 0)
	{
		ERR_error_string_n(ERR_get_error(), errmsg, sizeof(errmsg));
		WRITELOG("SSL_new()  %s\n", errmsg);
		return false;
	}

	if (!SSL_set_fd(_ssl, getSock()))
	{
		ERR_error_string_n(ERR_get_error(), errmsg, sizeof(errmsg));
		WRITELOG("SSL_set_fd()  %s\n", errmsg);
		SSL_free(_ssl);
	}

	SSL_set_options(_ssl, SSL_OP_NO_TICKET);

	if ( cert )
	{
		if ( SSL_use_certificate_file(_ssl, cert, SSL_FILETYPE_PEM) <= 0 )
		{
			ERR_error_string_n(ERR_get_error(), errmsg, sizeof(errmsg));
			WRITELOG("SSL_use_certificate_file() %s %s\n", cert, errmsg);
			SSL_free(_ssl);
			_ssl = 0;
			return false;
		}
	}
	if ( prvkey )
	{
		if ( SSL_use_PrivateKey_file(_ssl, prvkey, SSL_FILETYPE_PEM) <= 0 )
		{
			ERR_error_string_n(ERR_get_error(), errmsg, sizeof(errmsg));
			WRITELOG("SSL_use_PrivateKey_file() %s %s\n", prvkey, errmsg);
			SSL_free(_ssl);
			_ssl = 0;
			return false;
		}
	}

	if (!SSL_set_fd(_ssl, TCPStack::getSock()))
	{
		ERR_error_string_n(ERR_get_error(), errmsg, sizeof(errmsg));
		WRITELOG("SSL_set_fd()  %s\n", errmsg);
		SSL_free(_ssl);
		_ssl = 0;
		return false;
	}

	if (_session)
	{
		rc = SSL_set_session(_ssl, _session);
	}

	if (SSL_connect(_ssl) != 1)
	{
		ERR_error_string_n(ERR_get_error(), errmsg, sizeof(errmsg));
		WRITELOG("SSL_connect() %s\n", errmsg);
		SSL_free(_ssl);
		_ssl = 0;
		return false;
	}

	if ( (rc = SSL_get_verify_result(_ssl)) != X509_V_OK)
	{
		WRITELOG("SSL_get_verify_result() error: %s.\n", X509_verify_cert_error_string(rc));
		SSL_free(_ssl);
		_ssl = 0;
		return false;
	}

	X509* peer = SSL_get_peer_certificate(_ssl);
	X509_NAME_get_text_by_NID(X509_get_subject_name(peer), NID_commonName, peer_CN, 256);
	char* pos = peer_CN;
	if ( *pos == '*')
	{
		while (*host++ != '.');
		pos += 2;
	}
	if ( strcmp(host, pos))
	{
		WRITELOG("SSL_get_peer_certificate() error: Broker %s dosen't match the host name %s\n", peer_CN, host);
		SSL_free(_ssl);
		_ssl = 0;
		return false;
	}

	if (_session == 0)
	{
		_session = SSL_get1_session(_ssl);
	}
	_numOfInstance++;
	_sslValid = true;
	return true;
}

int Network::send(const uint8_t* buf, uint16_t length)
{
	char errmsg[256];
	fd_set rset;
	fd_set wset;
	bool writeBlockedOnRead = false;
	int bpos = 0;

	if (!_secureFlg)
	{
		return TCPStack::send(buf, length);
	}
	else
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
				if (FD_ISSET(getSock(), &wset) || (writeBlockedOnRead  && FD_ISSET(getSock(), &rset)))
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

	if (!_secureFlg)
	{
		return TCPStack::recv(buf, len);
	}

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
			WRITELOG("Network::recv() %s\n", errmsg);
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

void Network::close(void)
{
	if (_secureFlg)
	{
		_sslValid = false;
		SSL_free(_ssl);
		_ssl = 0;
	}
	TCPStack::close();
}

bool Network::isValid()
{
	if (_secureFlg)
	{
		if (_sslValid && !_busy)
		{
			return true;
		}
	}
	else
	{
		return TCPStack::isValid();
	}
	return false;
}

void Network::disconnect()
{
	if (_ssl)
	{
		SSL_shutdown(_ssl);
		_ssl = 0;
	}
	_sslValid = false;
	_busy = false;
	TCPStack::close();

}

int Network::getSock()
{
	return TCPStack::getSock();
}

bool Network::isSecure()
{
	return _secureFlg;
}


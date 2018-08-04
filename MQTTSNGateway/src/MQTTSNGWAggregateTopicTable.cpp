/**************************************************************************************
 * Copyright (c) 2018, Tomoaki Yamaguchi
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
#include "MQTTSNGWAggregateTopicTable.h"
#include "MQTTSNGWClient.h"

/*=====================================
 Class ClientTopicElement
 =====================================*/
ClientTopicElement::ClientTopicElement(Client* client)
{
	_client = client;
}

ClientTopicElement::~ClientTopicElement()
{

}

Client* ClientTopicElement::getClient(void)
{
	return _client;
}

/*=====================================
 Class AggregateTopicElement
 =====================================*/
AggregateTopicElement::AggregateTopicElement(void)
{

}

AggregateTopicElement::AggregateTopicElement(Topic* topic, Client* client)
{
	ClientTopicElement* elm = new ClientTopicElement(client);
	if ( elm != nullptr )
	{
		_head = elm;
		_tail = elm;
	}
}

AggregateTopicElement::~AggregateTopicElement(void)
{
	_mutex.lock();
	if ( _head != nullptr )
	{
		ClientTopicElement* p = _tail;
		while ( p )
		{
			ClientTopicElement* pPrev = p;
			delete p;
			p = pPrev->_prev;
		}
		_head = _tail = nullptr;
	}
	_mutex.unlock();
}

ClientTopicElement* AggregateTopicElement::add(Client* client)
{
	ClientTopicElement* elm = new ClientTopicElement(client);
	if ( elm == nullptr )
	{
		return nullptr;
	}
	_mutex.lock();
	if ( _head == nullptr )
	{
		_head = elm;
		_tail = elm;
	}
	else
	{
		ClientTopicElement* p = find(client);
		if ( p == nullptr )
		{
			 p = _tail;
			_tail = elm;
			elm->_prev = p;
			p->_next = elm;
		}
		else
		{
			delete elm;
			elm = nullptr;
		}
	}
	_mutex.unlock();
	return elm;
}

ClientTopicElement* AggregateTopicElement::find(Client* client)
{
	ClientTopicElement* p = _head;
	while ( p )
	{
		if ( p->_client == client)
		{
			break;
		}
		p = p->_next;
	}
	return p;
}

ClientTopicElement* AggregateTopicElement::getFirstElement(void)
{
	return _head;
}

ClientTopicElement* AggregateTopicElement::getNextElement(ClientTopicElement* elm)
{
	return elm->_next;
}


/*=====================================
 Class AggregateTopicTable
 ======================================*/

AggregateTopicTable::AggregateTopicTable()
{

}

AggregateTopicTable::~AggregateTopicTable()
{

}

AggregateTopicElement* AggregateTopicTable::add(Topic* topic, Client* client)
{
	//ToDo: AggregateGW
	return 0;
}

void AggregateTopicTable::remove(Topic* topic, Client* client)
{
	//ToDo: AggregateGW
}

AggregateTopicElement* AggregateTopicTable::getClientList(Topic* client)
{
	// ToDo: AggregateGW
	return 0;
}



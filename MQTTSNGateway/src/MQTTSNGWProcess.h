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

#ifndef MQTTSNGWPROCESS_H_
#define MQTTSNGWPROCESS_H_

#include <exception>
#include <string>
#include "MQTTSNGWDefines.h"
#include "Threading.h"

using namespace std;

namespace MQTTSNGW
{

/*=================================
 *    Parameters
 ==================================*/
#define MQTTSNGW_CONFIG_DIRECTORY "./"

#define MQTTSNGW_CONFIG_FILE      "param.conf"
#define MQTTSNGW_CLIENT_LIST      "clientList.conf"

#define MQTTSNGW_MAX_TASK           10  // number of Tasks
#define PROCESS_LOG_BUFFER_SIZE  16384  // Ring buffer size for Logs
#define MQTTSNGW_PARAM_MAX         128  // Max length of config records.

/*=================================
 *    Macros
 ==================================*/
#ifdef RINGBUFFER
#define WRITELOG theProcess->putLog
#else
#define WRITELOG printf
#endif


/*=================================
 Class Process
 ==================================*/
class Process
{
public:
	Process();
	virtual ~Process();
	virtual void initialize(int argc, char** argv);
	virtual void run(void);
	void putLog(const char* format, ...);
	void resetRingBuffer(void);
	int  getArgc(void);
	char** getArgv(void);
	int getParam(const char* parameter, char* value);
	const char* getLog(void);
	int checkSignal(void);

private:
	int _argc;
	char** _argv;
	RingBuffer* _rb;
	Semaphore*  _rbsem;
	Mutex _mt;
	char _rbdata[PROCESS_LOG_BUFFER_SIZE + 1];
};

/*=====================================
 Class MultiTaskProcess
 ====================================*/
class MultiTaskProcess: public Process
{
public:
	MultiTaskProcess(void);
	~MultiTaskProcess();
	virtual void initialize(int argc, char** argv);
	virtual int getParam(const char* parameter, char* value);
	void run(void);
	void attach(Thread* thread);
	Semaphore* getStopProcessEvent(void);

private:
	Thread* _threadList[MQTTSNGW_MAX_TASK];
	Semaphore _stopProcessEvent;
	Mutex _mutex;
	int _threadCount;
};

/*=====================================
 Class Exception
 =====================================*/
class Exception: public exception
{
public:
	Exception(const string& message);
	Exception(const int exNo, const string& message);
	Exception(const int exNo, const string& message,
			const char* file, const char* func, const int line);
	virtual ~Exception() throw ();
	const char* getFileName();
	const char* getFunctionName();
	const int getLineNo();
	const int getExceptionNo();
	virtual const char* what() const throw ();
	void writeMessage();

private:
	int _exNo;
	string _message;
	const char* _fileName;
	const char* _functionName;
	int _line;
};


/*=====================================
 Class QueElement
 ====================================*/
template<class T>
class QueElement
{
	template<class U> friend class Que;
public:
	QueElement(T* t)
	{
		_element = t;
		_next = 0;
		_prev = 0;
	}

	~QueElement()
	{
	}

private:
	T* _element;
	QueElement<T>* _next;
	QueElement<T>* _prev;
};

/*=====================================
 Class Que
 ====================================*/
template<class T>
class Que
{
public:
	Que()
	{
		_head = 0;
		_tail = 0;
		_cnt = 0;
	}

	~Que()
	{
		QueElement<T>* elm = _head;
		while (elm)
		{
			QueElement<T>* next = elm->_next;
			delete elm->_element;
			delete elm;
			elm = next;
		}
	}

	void pop(void)
	{
		if ( _head )
		{
			QueElement<T>* head = _head;
			if ( _head == _tail )
			{
				_head = _tail = 0;
			}
			else
			{
				_head = head->_next;
				head->_prev = 0;
			}
			delete head;
			_cnt--;
		}
	}

	T* front(void)
	{
		{
			if ( _head )
			{
				return _head->_element;
			}
			else
			{
				return 0;
			}
		}
	}

	int post(T* t)
	{
		QueElement<T>* elm = new QueElement<T>(t);
		if ( _head )
		{
			if ( _tail == _head )
			{
				elm->_prev = _tail;
				_tail = elm;
			}
			else
			{
			_tail->_next = elm;
			elm->_prev = _tail;
			_tail = elm;
			}
		}
		else
		{
			_head = elm;
			_tail = elm;
		}
		_cnt++;
		return _cnt;
	}

	int size(void)
	{
		return _cnt;
	}

private:
	int _cnt;
	QueElement<T>* _head;
	QueElement<T>* _tail;
};


extern Process* theProcess;
extern MultiTaskProcess* theMultiTaskProcess;

}
#endif /* MQTTSNGWPROCESS_H_ */

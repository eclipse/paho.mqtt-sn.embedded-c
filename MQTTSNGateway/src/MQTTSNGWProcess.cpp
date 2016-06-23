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
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>
#include <Timer.h>
#include <exception>
#include "MQTTSNGWProcess.h"
#include "Threading.h"

using namespace std;
using namespace MQTTSNGW;

char* currentDateTime(void);

/*=====================================
 Global Variables & Functions
 ======================================*/
Process* MQTTSNGW::theProcess = 0;
MultiTaskProcess* MQTTSNGW::theMultiTaskProcess = 0;

/*
 *  Save the type of signal
 */
volatile int theSignaled = 0;

static void signalHandler(int sig)
{
	theSignaled = sig;
}

/*=====================================
 Class Process
 ====================================*/
Process::Process()
{
	_argc = 0;
	_argv = 0;
	_rbsem = new Semaphore(MQTTSNGW_RB_SEMAPHOR_NAME, 0);
	_rb = new RingBuffer();
}

Process::~Process()
{
	delete _rb;
	delete _rbsem;
}

void Process::run()
{

}

void Process::initialize(int argc, char** argv)
{
	_argc = argc;
	_argv = argv;
	signal(SIGINT, signalHandler);
	signal(SIGTERM, signalHandler);
	signal(SIGHUP, signalHandler);
}

int Process::getArgc()
{
	return _argc;
}

char** Process::getArgv()
{
	return _argv;
}

int Process::getParam(const char* parameter, char* value)
{
	char str[MQTTSNGW_PARAM_MAX];
	char param[MQTTSNGW_PARAM_MAX];
	FILE *fp;
	int i = 0, j = 0;

	if ((fp = fopen(MQTTSNGW_CONFIG_FILE, "r")) == NULL)
	{
		WRITELOG("No config file:[%s]\n", MQTTSNGW_CONFIG_FILE);
		return -1;
	}

	while (true)
	{
		if (fgets(str, MQTTSNGW_PARAM_MAX - 1, fp) == NULL)
		{
			fclose(fp);
			return -3;
		}
		if (!strncmp(str, parameter, strlen(parameter)))
		{
			while (str[i++] != '=')
			{
				;
			}
			while (str[i] != '\n')
			{
				param[j++] = str[i++];
			}
			param[j] = '\0';

			for (i = strlen(param) - 1; i >= 0 && isspace(param[i]); i--)
				;
			param[i + 1] = '\0';
			for (i = 0; isspace(param[i]); i++)
				;
			if (i > 0)
			{
				j = 0;
				while (param[i])
					param[j++] = param[i++];
				param[j] = '\0';
			}
			strcpy(value, param);
			fclose(fp);
			return 0;
		}
	}
	fclose(fp);
	return -2;
}

void Process::putLog(const char* format, ...)
{
	_mt.lock();
	va_list arg;
	va_start(arg, format);
	vsprintf(_rbdata, format, arg);
	va_end(arg);
	if (strlen(_rbdata))
	{
		_rb->put(_rbdata);
		_rbsem->post();
	}
	_mt.unlock();
}

const char* Process::getLog()
{
	int len = 0;
	_mt.lock();
	while ((len = _rb->get(_rbdata, PROCESS_LOG_BUFFER_SIZE)) == 0)
	{
		_rbsem->timedwait(1000);
		if ( checkSignal() == SIGINT)
		{
			break;
		}
	}
	*(_rbdata + len) = 0;
	_mt.unlock();
	return _rbdata;
}

void Process::resetRingBuffer()
{
	_rb->reset();
}

int Process::checkSignal(void)
{
	return theSignaled;
}

/*=====================================
 Class MultiTaskProcess
 ====================================*/
MultiTaskProcess::MultiTaskProcess()
{
	theMultiTaskProcess = this;
	_threadCount = 0;
}

MultiTaskProcess::~MultiTaskProcess()
{
	for (int i = 0; i < _threadCount; i++)
	{
		if ( _threadList[i] )
		{
			delete _threadList[i];
		}
	}
}

void MultiTaskProcess::initialize(int argc, char** argv)
{
	Process::initialize(argc, argv);
	for (int i = 0; i < _threadCount; i++)
	{
		_threadList[i]->initialize(argc, argv);
	}

}

void MultiTaskProcess::run(void)
{
	for (int i = 0; i < _threadCount; i++)
	{
		_threadList[i]->start();
	}

	try
	{
		_stopProcessEvent.wait();
	}
	catch (Exception* ex)
	{
		ex->writeMessage();
	}
}

Semaphore* MultiTaskProcess::getStopProcessEvent(void)
{
	return &_stopProcessEvent;
}

void MultiTaskProcess::attach(Thread* thread)
{
	if (_threadCount < MQTTSNGW_MAX_TASK)
	{
		_threadList[_threadCount] = thread;
		_threadCount++;
	}
	else
	{
		throw Exception("Full of Threads");
	}
}

int MultiTaskProcess::getParam(const char* parameter, char* value)
{
	_mutex.lock();
	int rc = Process::getParam(parameter, value);
	_mutex.unlock();
	if (rc == -1)
	{
		throw Exception("No config file.");
	}
	return rc;
}

/*=====================================
 Class Exception
 ======================================*/
Exception::Exception(const string& message)
{
	_message = message;
	_exNo = 0;
	_fileName = 0;
	_functionName = 0;
	_line = 0;
}

Exception::Exception(const int exNo, const string& message)
{
	_message = message;
	_exNo = exNo;
	_fileName = 0;
	_functionName = 0;
	_line = 0;
}

Exception::Exception(const int exNo, const string& message, const char* file,
		const char* function, const int line)
{
	_message = message;
	_exNo = exNo;
	_fileName = file;
	_functionName = function;
	_line = line;
}

Exception::~Exception() throw ()
{

}

const char* Exception::what() const throw ()
{
	return _message.c_str();
}

const char* Exception::getFileName()
{
	return _fileName;
}

const char* Exception::getFunctionName()
{
	return _functionName;
}

const int Exception::getLineNo()
{
	return _line;
}

const int Exception::getExceptionNo()
{
	return _exNo;
}

void Exception::writeMessage()
{
	if (getExceptionNo() == 0 )
	{
		WRITELOG("%s : %s\n", currentDateTime(), what());
	}
	else
	{
		WRITELOG("%s:%-6d   %s  line %-4d %s() : %s\n", currentDateTime(), getExceptionNo(),
			getFileName(), getLineNo(), getFunctionName(), what());
	}
}

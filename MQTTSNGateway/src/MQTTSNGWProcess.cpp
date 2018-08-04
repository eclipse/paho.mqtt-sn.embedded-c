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
#include <getopt.h>
#include <unistd.h>
#include "MQTTSNGWProcess.h"
#include "Threading.h"

using namespace std;
using namespace MQTTSNGW;

char* currentDateTime(void);

/*=====================================
 Global Variables & Functions
 ======================================*/
Process* MQTTSNGW::theProcess = nullptr;
MultiTaskProcess* MQTTSNGW::theMultiTaskProcess = nullptr;

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
	_configDir = CONFIG_DIRECTORY;
	_configFile = CONFIG_FILE;
	_log = 0;
}

Process::~Process()
{
	if (_rb )
	{
		delete _rb;
	}
	if ( _rbsem )
	{
		delete _rbsem;
	}
}

void Process::run()
{

}

void Process::initialize(int argc, char** argv)
{
	char param[MQTTSNGW_PARAM_MAX];
	_argc = argc;
	_argv = argv;
	signal(SIGINT, signalHandler);
	signal(SIGTERM, signalHandler);
	signal(SIGHUP, signalHandler);

	int opt;
	while ((opt = getopt(_argc, _argv, "f:")) != -1)
	{
		if ( opt == 'f' )
		{
			string config = string(optarg);
			size_t pos = 0;
			if ( (pos = config.find_last_of("/")) == string::npos )
			{
				_configFile = optarg;
			}
			else
			{
				_configFile = config.substr(pos + 1, config.size() - pos - 1);;
				_configDir = config.substr(0, pos + 1);
			}
		}
	}
	_rbsem = new Semaphore(MQTTSNGW_RB_SEMAPHOR_NAME, 0);
	_rb = new RingBuffer(_configDir.c_str());

	if (getParam("ShearedMemory", param) == 0)
	{
		if (!strcasecmp(param, "YES"))
		{
			_log = 1;
		}
		else
		{
			_log = 0;
		}
	}
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
		if ( _log > 0 )
		{
			_rb->put(_rbdata);
			_rbsem->post();
		}
		else
		{
			printf("%s", _rbdata);
		}
	}
	_mt.unlock();
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
	string configPath = _configDir + _configFile;

	if ((fp = fopen(configPath.c_str(), "r")) == NULL)
	{
		WRITELOG("No config file:[%s]\n", configPath.c_str());
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

const string* Process::getConfigDirName(void)
{
	return &_configDir;
}

const string* Process::getConfigFileName(void)
{
	return &_configFile;
}

/*=====================================
 Class MultiTaskProcess
 ====================================*/
MultiTaskProcess::MultiTaskProcess()
{
	theMultiTaskProcess = this;
	_threadCount = 0;
	_stopCount = 0;
}

MultiTaskProcess::~MultiTaskProcess()
{
	for (int i = 0; i < _threadCount; i++)
	{
		_threadList[i]->stop();
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
		while(true)
		{
			if (theProcess->checkSignal() == SIGINT)
			{
				return;
			}
			sleep(1);
		}
	}
	catch(Exception* ex)
	{
		ex->writeMessage();
	}
	catch(...)
	{
		throw;
	}
}

void MultiTaskProcess::waitStop(void)
{
	while (_stopCount < _threadCount)
	{
		sleep(1);
	}
}

void MultiTaskProcess::threadStoped(void)
{
	_mutex.lock();
	_stopCount++;
	_mutex.unlock();

}

void MultiTaskProcess::attach(Thread* thread)
{
	_mutex.lock();
	if (_threadCount < MQTTSNGW_MAX_TASK)
	{
		_threadList[_threadCount] = thread;
		_threadCount++;
	}
	else
	{
		_mutex.unlock();
		throw Exception("Full of Threads");
	}
	_mutex.unlock();
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
	_fileName = nullptr;
	_functionName = nullptr;
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
		WRITELOG("%s %s\n", currentDateTime(), what());
	}
	else
	{
		WRITELOG("%s:%-6d   %s  line %-4d %s() : %s\n", currentDateTime(), getExceptionNo(),
			getFileName(), getLineNo(), getFunctionName(), what());
	}
}

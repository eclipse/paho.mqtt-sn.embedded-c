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

#include "MQTTSNGWProcess.h"
#include "Threading.h"
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

using namespace std;
using namespace MQTTSNGW;

#if defined(OSX)
int sem_timedwait(sem_type sem, const struct timespec *timeout)
{
	int rc = -1;
	int64_t tout = timeout->tv_sec * 1000L + tv_nsec * 1000000L
	rc = (int)dispatch_semaphore_wait(sem, dispatch_time(DISPATCH_TIME_NOW, tout));
	if (rc != 0)
	{
		rc = ETIMEDOUT;
	}
 	return rc;
}
#endif

/*=====================================
 Class Mutex
 =====================================*/

Mutex::Mutex(void)
{
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutex_init(&_mutex, &attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
	_shmid = 0;
	_pmutex = 0;
}

Mutex::Mutex(const char* fileName)
{
	pthread_mutexattr_t attr;

	key_t key = ftok(fileName, 1);

	if ((_shmid = shmget(key, sizeof(pthread_mutex_t), IPC_CREAT | 0666)) < 0)
	{
		throw Exception( -1, "Mutex can't create a shared memory.");
	}
	_pmutex = (pthread_mutex_t*) shmat(_shmid, NULL, 0);
	if (_pmutex < 0)
	{
		throw Exception( -1, "Mutex can't attach shared memory.");
	}

	pthread_mutexattr_init(&attr);

	if (pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED) != 0)
	{
		throw Exception( -1, "Mutex can't set the process-shared flag");
	}
	if (pthread_mutex_init(_pmutex, &attr) != 0)
	{
		throw Exception( -1, "Mutex can't initialize.");
	}
}

Mutex::~Mutex(void)
{
	if (_pmutex)
	{
		pthread_mutex_lock(_pmutex);
		pthread_mutex_unlock(_pmutex);
		pthread_mutex_destroy(_pmutex);
	}
	else
	{
		pthread_mutex_lock(&_mutex);
		pthread_mutex_unlock(&_mutex);
		pthread_mutex_destroy(&_mutex);
	}
	if (_shmid)
	{
		shmctl(_shmid, IPC_RMID, NULL);
	}
}

void Mutex::lock(void)
{
	if (_pmutex)
	{
		pthread_mutex_lock(_pmutex);
	}
	else
	{
		try
		{
			if (pthread_mutex_lock(&_mutex))
			{
				throw;
			}
		} catch (char* errmsg)
		{
			throw Exception( -1, "The same thread can't aquire a mutex twice.");
		}
	}
}

void Mutex::unlock(void)
{

	if (_pmutex)
	{
		pthread_mutex_unlock(_pmutex);
	}
	else
	{
		try
		{
			if (pthread_mutex_unlock(&_mutex))
			{
				throw;
			}
		} catch (char* errmsg)
		{
			throw Exception( -1, "Mutex can't unlock.");
		}
	}
}

/*=====================================
 Class Semaphore
 =====================================*/

Semaphore::Semaphore()
{
	sem_init(&_sem, 0, 0);
	_name = 0;
	_psem = 0;
}

Semaphore::Semaphore(unsigned int val)
{
	sem_init(&_sem, 0, val);
	_name = 0;
	_psem = 0;
}

Semaphore::Semaphore(const char* name, unsigned int val)
{
	_psem = sem_open(name, O_CREAT, 0666, val);
	if (_psem == SEM_FAILED)
	{
		throw Exception( -1, "Semaphore can't be created.");
	}
	_name = strdup(name);
	if (_name == NULL)
	{
		throw Exception( -1, "Semaphore can't allocate memories.");
	}
}

Semaphore::~Semaphore()
{
	if (_name)
	{
		sem_close(_psem);
		sem_unlink(_name);
		free(_name);
	}
	else
	{
		sem_destroy(&_sem);
	}
}

void Semaphore::post(void)
{
	int val = 0;
	if (_psem)
	{
		sem_getvalue(_psem, &val);
		if (val <= 0)
		{
			sem_post(_psem);
		}
	}
	else
	{
		sem_getvalue(&_sem, &val);
		if (val <= 0)
		{
			sem_post(&_sem);
		}
	}
}

void Semaphore::wait(void)
{
	if (_psem)
	{
		sem_wait(_psem);
	}
	else
	{
		sem_wait(&_sem);
	}
}

void Semaphore::timedwait(uint16_t millsec)
{
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += millsec / 1000;
	ts.tv_nsec = (millsec % 1000) * 1000000;
	if (_psem)
	{
		sem_timedwait(_psem, &ts);
	}
	else
	{
		sem_timedwait(&_sem, &ts);
	}
}

/*=========================================
 Class RingBuffer
 =========================================*/
RingBuffer::RingBuffer()
{
	RingBuffer(MQTTSNGW_KEY_DIRECTORY);
}

RingBuffer::RingBuffer(const char* keyDirectory)
{
	int fp = 0;
	string fileName = keyDirectory + string(MQTTSNGW_RINGBUFFER_KEY);
	fp = open(fileName.c_str(), O_CREAT, S_IRGRP);
	if ( fp > 0 )
	{
		close(fp);
	}

	fileName = keyDirectory + string(MQTTSNGW_RB_MUTEX_KEY);
	fp = open(fileName.c_str(), O_CREAT, S_IRGRP);
	if ( fp > 0 )
	{
		close(fp);
	}

	key_t key = ftok(MQTTSNGW_RINGBUFFER_KEY, 1);

	if ((_shmid = shmget(key, PROCESS_LOG_BUFFER_SIZE,
	IPC_CREAT | IPC_EXCL | 0666)) >= 0)
	{
		if ((_shmaddr = (uint16_t*) shmat(_shmid, NULL, 0)) > 0)
		{
			_length = (uint16_t*) _shmaddr;
			_start = (uint16_t*) _length + sizeof(uint16_t*);
			_end = (uint16_t*) _start + sizeof(uint16_t*);
			_buffer = (char*) _end + sizeof(uint16_t*);
			_createFlg = true;

			*_length = PROCESS_LOG_BUFFER_SIZE - sizeof(uint16_t*) * 3 - 16;
			*_start = *_end = 0;
		}
		else
		{
			throw Exception(-1, "RingBuffer can't attach shared memory.");
		}
	}
	else if ((_shmid = shmget(key, PROCESS_LOG_BUFFER_SIZE, IPC_CREAT | 0666)) >= 0)
	{
		if ((_shmaddr = (uint16_t*) shmat(_shmid, NULL, 0)) > 0)
		{
			_length = (uint16_t*) _shmaddr;
			_start = (uint16_t*) _length + sizeof(uint16_t*);
			_end = (uint16_t*) _start + sizeof(uint16_t*);
			_buffer = (char*) _end + sizeof(uint16_t*);
			_createFlg = false;
		}
		else
		{
			throw Exception(-1, "RingBuffer can't create a shared memory.");
		}
	}
	else
	{
		throw Exception(-1, "RingBuffer can't create a shared memory.");
	}

	_pmx = new Mutex(MQTTSNGW_RB_MUTEX_KEY);
}

RingBuffer::~RingBuffer()
{
	if (_createFlg)
	{
		if (_shmid > 0)
		{
			shmctl(_shmid, IPC_RMID, NULL);
		}
	}
	else
	{
		if (_shmid > 0)
		{
			shmdt(_shmaddr);
		}
	}

	if (_pmx > 0)
	{
		delete _pmx;
	}
}

void RingBuffer::put(char* data)
{
	_pmx->lock();

	uint16_t dlen = strlen(data);
	uint16_t blen = *_length - *_end;

	if (*_end > *_start)
	{
		if (dlen < blen)
		{
			strncpy(_buffer + *_end, data, dlen);
			if (*_end - *_start == 1)
			{ // Buffer is empty.
				*_start = *_end;
			}
			*_end += dlen;
		}
		else
		{
			strncpy(_buffer + *_end, data, blen);
			strncpy(_buffer, data + blen, dlen - blen);
			if (*_end - *_start == 1)
			{ // Buffer is empty.
				*_start = *_end;
				*_end = dlen - blen;
			}
			else
			{
				*_end = dlen - blen;
				*_start = *_end + 1;
			}
		}
	}
	else if (*_end == *_start)
	{
		if (dlen < blen)
		{
			strncpy(_buffer + *_end, data, dlen);
			*_end += dlen;
		}
		else
		{
			const char* errmsg = "RingBuffer Error: data is too long";
			strcpy(_buffer + *_end, errmsg);
			*_end += strlen(errmsg);
		}
	}
	else
	{    // *_end < *_start
		if (dlen < *_start - *_end)
		{
			strncpy(_buffer + *_end, data, dlen);
			*_end += dlen;
			*_start = *_end + 1;
		}
		else
		{
			if (dlen < blen)
			{
				strncpy(_buffer + *_end, data, dlen);
				*_end += dlen;
				*_start = *_end + 1;
			}
			else
			{
				strncpy(_buffer + *_end, data, blen);
				strncpy(_buffer, data + blen, dlen - blen);
				*_start = *_end;
				*_end = dlen - blen;
			}
		}
	}
	_pmx->unlock();
}

int RingBuffer::get(char* buf, int length)
{
	int len = 0;
	_pmx->lock();

	if (*_end > *_start)
	{
		if (length > *_end - *_start)
		{
			len = *_end - *_start;
			if (len == 1)
			{
				len = 0;
			}
			strncpy(buf, _buffer + *_start, len);
			*_start = *_end - 1;
		}
		else
		{
			len = length;
			strncpy(buf, _buffer + *_start, len);
			*_start = *_start + len;
		}
	}
	else if (*_end < *_start)
	{
		int blen = *_length - *_start;
		if (length > blen)
		{
			strncpy(buf, _buffer + *_start, blen);
			*_start = 0;
			if (length - (blen + *_end) > 0)
			{
				strncpy(buf + blen, _buffer, *_end);
				len = blen + *_end;
				if (*_end > 0)
				{
					*_start = *_end - 1;
				}
			}
			else
			{
				strncpy(buf + blen, _buffer, length - blen);
				len = length;
				*_start = length - blen;
			}
		}
		else
		{
			strncpy(buf, _buffer + *_start, length);
			*_start += length;
			len = length;
		}
	}
	_pmx->unlock();
	return len;
}

void RingBuffer::reset()
{
	_pmx->lock();
	if ( _start && _end )
	{
		*_start = *_end = 0;
	}
	else
	{
		throw Exception(-1, "RingBuffer can't reset. need to clear shared memory.");
	}
	_pmx->unlock();
}

/*=====================================
 Class Thread
 =====================================*/
Thread::Thread()
{
	_threadID = 0;
}

Thread::~Thread()
{
}

void* Thread::_run(void* runnable)
{
	static_cast<Runnable*>(runnable)->EXECRUN();
	return 0;
}

void Thread::initialize(int argc, char** argv)
{

}

pthread_t Thread::getID()
{
	return pthread_self();
}

bool Thread::equals(pthread_t *t1, pthread_t *t2)
{
	return (pthread_equal(*t1, *t2) ? false : true);
}

int Thread::start(void)
{
	Runnable* runnable = this;
	return pthread_create(&_threadID, 0, _run, runnable);
}

void Thread::stopProcess(void)
{
	theMultiTaskProcess->threadStoped();
}

void Thread::stop(void)
{
	if ( _threadID )
	{
		pthread_join(_threadID, NULL);
		_threadID = 0;
	}
}

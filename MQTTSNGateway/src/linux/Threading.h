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

#ifndef THREADING_H_
#define THREADING_H_

#include <pthread.h>
#include <semaphore.h>
#include "MQTTSNGWDefines.h"

namespace MQTTSNGW
{
#define MQTTSNGW_KEY_DIRECTORY "./"
#define MQTTSNGW_RINGBUFFER_KEY   "ringbuffer.key"
#define MQTTSNGW_RB_MUTEX_KEY     "rbmutex.key"
#define MQTTSNGW_RB_SEMAPHOR_NAME "/rbsemaphor"

/*=====================================
         Class Mutex
  ====================================*/
class Mutex
{
public:
	Mutex();
	Mutex(const char* name);
	~Mutex();
	void lock(void);
	void unlock(void);

private:
	pthread_mutex_t _mutex;
	pthread_mutex_t* _pmutex;
	int   _shmid;
};

/*=====================================
         Class Semaphore
  ====================================*/
class Semaphore
{
public:
	Semaphore();
	Semaphore(unsigned int val);
	Semaphore(const char* name, unsigned int val);
	~Semaphore();
	void post(void);
	void wait(void);
	void timedwait(uint16_t millsec);

private:
	sem_t* _psem;
	sem_t  _sem;
	char*  _name;
};

/*=====================================
        Class RingBuffer
 =====================================*/
class RingBuffer
{
public:
	RingBuffer();
	RingBuffer(const char* keyDirctory);
	~RingBuffer();
	void put(char* buffer);
	int get(char* buffer, int bufferLength);
	void reset();
private:
	void* _shmaddr;
	uint16_t* _length;
	uint16_t* _start;
	uint16_t* _end;
	char* _buffer;
	int _shmid;
	Mutex* _pmx;
	bool _createFlg;
};


/*=====================================
         Class Runnable
  ====================================*/
class Runnable
{
public:
	Runnable(){}
	virtual ~Runnable(){}
	virtual void EXECRUN(){}
};

#define MAGIC_WORD_FOR_THREAD \
public: void EXECRUN() \
{ \
	try \
	{ \
		run(); \
		stopProcess(); \
	} \
	catch(...) \
	{ \
		throw; \
	} \
}

/*=====================================
         Class Thread
  ====================================*/
class Thread : virtual public Runnable{
public:
	Thread();
    ~Thread();
	int start(void);
	static pthread_t getID();
	static bool equals(pthread_t*, pthread_t*);
	virtual void initialize(int argc, char** argv);
	void stopProcess(void);
	void waitStop(void);
	void stop(void);
private:
	static void* _run(void*);
	pthread_t _threadID;
};

}

#endif /* THREADING_H_ */

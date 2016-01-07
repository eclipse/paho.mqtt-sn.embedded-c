

extern "C"
{
	#include "Thread.h"
}


class Thread
{

/*
Thread(void (*fn)(void const *argument), void *argument)
{
	Thread_start(fn, arg);
}*/

public:

	Thread(void (*fn)(void const *argument))
	{
		const void* arg = NULL;
	
		//Thread_start((void (*)(void *))fn, arg);
	}

};




class Mutex
{


};

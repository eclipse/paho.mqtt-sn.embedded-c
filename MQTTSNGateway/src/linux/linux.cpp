/**************************************************************************************
 * Copyright (c) 2016, Tomy Tech Corp.
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
#ifndef LINUX_CPP_
#define LINUX_CPP_

#include <stdio.h>
#include <time.h>
#include <sys/time.h>

#include "MQTTSNGWDefines.h"

extern "C"
{
int wiringPiSetup(void);
void pinMode(int gpioNo, int mode);
void digitalWrite(int gpioNo, int val);
}

using namespace std;

namespace MQTTSNGW
{
/*==========================================================
 *           Light Indicators
 ===========================================================*/
//#define RASPBERRY_PI
#define LIGHT_INDICATOR_GREEN   4    // RPi connector 16
#define LIGHT_INDICATOR_RED     5    // RPi connector 18
#define LIGHT_INDICATOR_BLUE    6    // RPi connector 22

#ifdef RASPBERRY_PI
#include <wiringPi.h>
#endif

/*============================================
 Timer
 ============================================*/
class Timer
{
public:
	Timer(void)
	{
		stop();
	}

	~Timer(void)
	{

	}

	void start(uint32_t msecs = 0)
	{
		gettimeofday(&_startTime, 0);
		_millis = msecs;
	}

	bool isTimeup(void)
	{
		return isTimeup(_millis);
	}

	bool isTimeup(uint32_t msecs)
	{
		struct timeval curTime;
		long secs, usecs;
		if (_startTime.tv_sec == 0)
		{
			return false;
		}
		else
		{
			gettimeofday(&curTime, 0);
			secs = (curTime.tv_sec - _startTime.tv_sec) * 1000;
			usecs = (curTime.tv_usec - _startTime.tv_usec) / 1000.0;
			return ((secs + usecs) > (long) msecs);
		}
	}

	void stop()
	{
		_startTime.tv_sec = 0;
		_millis = 0;
	}
private:
	struct timeval _startTime;
	uint32_t _millis;
};

/*=====================================
 Class LightIndicator
 =====================================*/
class LightIndicator
{
public:
	LightIndicator()
	{
		_gpioAvailable = false;
		init();
		_greenStatus = true;
		_blueStatus = true;
		greenLight(false);
		blueLight(false);
	}

	~LightIndicator()
	{

	}
	void greenLight(bool on)
	{
		if (on)
		{
			if (_greenStatus == 0)
			{
				_greenStatus = 1;
				//Turn Green on & turn Red off
				lit(LIGHT_INDICATOR_GREEN, 1);
				lit(LIGHT_INDICATOR_RED, 0);
			}
		}
		else
		{
			if (_greenStatus == 1)
			{
				_greenStatus = 0;
				//Turn Green off & turn Red on
				lit(LIGHT_INDICATOR_GREEN, 0);
				lit(LIGHT_INDICATOR_RED, 1);
			}
		}
	}
	void blueLight(bool on)
	{
		if (on)
		{
			if (_greenStatus == 0)
			{
				_greenStatus = 1;
				//Turn Green on & turn Red off
				lit(LIGHT_INDICATOR_GREEN, 1);
				lit(LIGHT_INDICATOR_RED, 0);
			}
		}
		else
		{
			if (_greenStatus == 1)
			{
				_greenStatus = 0;
				//Turn Green off & turn Red on
				lit(LIGHT_INDICATOR_GREEN, 0);
				lit(LIGHT_INDICATOR_RED, 1);
			}
		}
	}

	void redLightOff(void)
	{
		lit(LIGHT_INDICATOR_RED, 0);
	}
private:
	void init()
	{
#ifdef RASPBERRY_PI
		if(wiringPiSetup() != -1)
		{
			pinMode(LIGHT_INDICATOR_GREEN, OUTPUT);
			pinMode(LIGHT_INDICATOR_RED, OUTPUT);
			pinMode(LIGHT_INDICATOR_BLUE, OUTPUT);
			_gpioAvailable = true;
		}
#endif
	}

	void lit(int gpioNo, int onoff)
	{
#ifdef RASPBERRY_PI
		if(_gpioAvailable)
		{
			digitalWrite(gpioNo,onoff);
		}
#endif
	}

	bool _greenStatus;
	bool _blueStatus;
	bool _gpioAvailable;
};

}

#endif  /* LINUX_CPP_ */

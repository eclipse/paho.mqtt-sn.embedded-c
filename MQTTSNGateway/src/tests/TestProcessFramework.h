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
 *    Tomoaki Yamaguchi - initial API and implementation 
 **************************************************************************************/
#ifndef TESTPROCESSFRAMEWORK_H_
#define TESTPROCESSFRAMEWORK_H_

#include "MQTTSNGWProcess.h"


namespace MQTTSNGW
{
class TestProcessFramework: public MultiTaskProcess{
public:
	TestProcessFramework();
	~TestProcessFramework();
	virtual void initialize(int argc, char** argv);
	void run(void);

private:

};

class TestQue
{
public:
	TestQue();
	~TestQue();
	void post(int*);
	int* front(void);
	void pop(void);
	int size(void);
	void setMaxSize(int maxsize);
private:
	Que<int> _que;
};
}

#endif /* TESTPROCESSFRAMEWORK_H_ */

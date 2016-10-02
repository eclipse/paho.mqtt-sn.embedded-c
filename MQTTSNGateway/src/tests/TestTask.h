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
#ifndef TESTTASK_H_
#define TESTTASK_H_

#include "Threading.h"
#include "TestProcessFramework.h"

namespace MQTTSNGW
{

class TestTask: public Thread
{
MAGIC_WORD_FOR_THREAD;
	;
public:
	TestTask(TestProcessFramework* proc);
	~TestTask();
	void initialize(int argc, char** argv);
	void run(void);

private:

};

}


#endif /* TESTTASK_H_ */

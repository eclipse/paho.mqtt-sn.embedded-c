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
#include <unistd.h>
#include "TestTask.h"
#include "Threading.h"

using namespace std;
using namespace MQTTSNGW;

TestTask::TestTask(TestProcessFramework* proc)
{
	proc->attach((Thread*)this);
}

TestTask::~TestTask()
{

}

void TestTask::initialize(int argc, char** argv)
{
	printf("Task initialize complite.\n");
}

void TestTask::run(void)
{
	while(true)
	{
		if ( CHK_SIGINT)
		{
			printf("Task stopped.\n");
			return;
		}
		printf("Task is running. Enter CTRL+C\n");
		sleep(1);
	}
}

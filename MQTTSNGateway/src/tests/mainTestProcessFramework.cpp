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
#include "TestProcessFramework.h"
#include "TestTask.h"

using namespace MQTTSNGW;

TestProcessFramework* proc = new TestProcessFramework();
TestTask* task = new TestTask(proc);

int main(int argc, char** argv)
{
	proc->initialize(argc, argv);
	proc->run();
	delete proc;
	return 0;
}


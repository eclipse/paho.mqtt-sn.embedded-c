/*******************************************************************************
 * Copyright (c) 2014 IBM Corp.
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
 *    Ian Craggs - initial API and implementation and/or initial documentation
 *******************************************************************************/



int MQTTDeserialize_subscribe(int* dup, int* packetid, int maxcount, int* count, MQTTString topicFilters[], int requestedQoSs[],
    char* buf, int buflen)
{

}


int MQTTSerialize_suback(char* buf, int buflen, int packetid, int count, int* grantedQoSs)
{


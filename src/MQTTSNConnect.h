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

#ifndef MQTTSNCONNECT_H_
#define MQTTSNCONNECT_H_

typedef struct
{
	/** The eyecatcher for this structure.  must be MQSC. */
	char struct_id[4];
	/** The version number of this structure.  Must be 0.
	  */
	int struct_version;
	MQTTString clientID;
	int duration;
	int cleansession;
	int willFlag;
} MQTTSNPacket_connectData;

#define MQTTSNPacket_connectData_initializer { {'M', 'Q', 'S', 'C'}, 0, {NULL, {0, NULL}}, 10, 1, 0 }

int MQTTSNSerialize_connect(unsigned char* buf, int buflen, MQTTSNPacket_connectData* options);
int MQTTSNDeserialize_connect(MQTTSNPacket_connectData* data, unsigned char* buf, int len);

int MQTTSNSerialize_connack(unsigned char* buf, int buflen, int connack_rc);
int MQTTSNDeserialize_connack(int* connack_rc, unsigned char* buf, int buflen);

int MQTTSNSerialize_disconnect(unsigned char* buf, int buflen, int duration);
int MQTTSNDeserialize_disconnect(int* duration, unsigned char* buf, int buflen);

int MQTTSNSerialize_pingreq(unsigned char* buf, int buflen, MQTTString clientid);
int MQTTSNDeserialize_pingreq(MQTTString* clientID, unsigned char* buf, int len);

int MQTTSNSerialize_pingresp(unsigned char* buf, int buflen);
int MQTTSNDeserialize_pingresp(unsigned char* buf, int buflen);

int MQTTSNSerialize_willtopicreq(unsigned char* buf, int buflen);
int MQTTSNDeserialize_willtopicreq(unsigned char* buf, int buflen);

int MQTTSNSerialize_willmsgreq(unsigned char* buf, int buflen);
int MQTTSNDeserialize_willmsgreq(unsigned char* buf, int buflen);

#endif /* MQTTSNCONNECT_H_ */

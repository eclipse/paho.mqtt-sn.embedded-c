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

#include "StackTrace.h"
#include "MQTTSNPacket.h"
#include <string.h>

#define min(a, b) ((a < b) ? 1 : 0)

/**
  * Deserializes the supplied (wire) buffer into publish data
  * @param dup returned integer - the MQTT dup flag
  * @param qos returned integer - the MQTT QoS value
  * @param retained returned integer - the MQTT retained flag
  * @param packetid returned integer - the MQTT packet identifier
  * @param topicName returned MQTTString - the MQTT topic in the publish
  * @param payload returned byte buffer - the MQTT publish payload
  * @param payloadlen returned integer - the length of the MQTT payload
  * @param buf the raw buffer data, of the correct length determined by the remaining length field
  * @param buflen the length in bytes of the data in the supplied buffer
  * @return error code.  1 is success
  */
int MQTTSNDeserialize_publish(int* dup, int* qos, int* retained, int* packetid, MQTTSN_topicid* topic,
		unsigned char** payload, int* payloadlen, unsigned char* buf, int buflen)
{
	MQTTSNFlags flags;
	unsigned char* curdata = buf;
	unsigned char* enddata = NULL;
	int rc = 0;
	int mylen = 0;

	FUNC_ENTRY;
	curdata += (rc = MQTTSNPacket_decodeBuf(curdata, &mylen)); /* read length */
	enddata = buf + mylen;
	if (enddata - curdata > buflen)
		goto exit;

	if (readChar(&curdata) != MQTTSN_PUBLISH)
		goto exit;

	flags.all = readChar(&curdata);
	*dup = flags.bits.dup;
	*qos = flags.bits.QoS;
	*retained = flags.bits.retain;

	topic->type = flags.bits.topicIdType;
	if (topic->type == MQTTSN_TOPIC_TYPE_NORMAL && *qos == 3)
	{
		/* special arrangement for long topic names in QoS -1 publishes.  The length of the topic is in the topicid field */
		topic->data.qos3.longlen = readInt(&curdata);
	}
	else if (topic->type == MQTTSN_TOPIC_TYPE_NORMAL || topic->type == MQTTSN_TOPIC_TYPE_PREDEFINED)
		topic->data.id = readInt(&curdata);
	else
	{
		topic->data.name[0] = readChar(&curdata);
		topic->data.name[1] = readChar(&curdata);
	}
	*packetid = readInt(&curdata);

	if (topic->type == MQTTSN_TOPIC_TYPE_NORMAL && *qos == 3)
	{
		topic->data.qos3.longname = (char*)curdata;
		curdata += topic->data.qos3.longlen;
	}

	*payloadlen = enddata - curdata;
	*payload = curdata;
	rc = 1;
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}

#if 0

/**
  * Deserializes the supplied (wire) buffer into an ack
  * @param type returned integer - the MQTT packet type
  * @param dup returned integer - the MQTT dup flag
  * @param packetid returned integer - the MQTT packet identifier
  * @param buf the raw buffer data, of the correct length determined by the remaining length field
  * @param buflen the length in bytes of the data in the supplied buffer
  * @return error code.  1 is success, 0 is failure
  */
int MQTTDeserialize_ack(int* type, int* dup, int* packetid, char* buf, int buflen)
{
	MQTTHeader header;
	char* curdata = buf;
	char* enddata = NULL;
	int rc = 0;
	int mylen;

	FUNC_ENTRY;
	header.byte = readChar(&curdata);
	*dup = header.bits.dup;
	*type = header.bits.type;

	curdata += (rc = MQTTPacket_decodeBuf(curdata, &mylen)); /* read remaining length */
	enddata = curdata + mylen;

	if (enddata - curdata < 2)
		goto exit;
	*packetid = readInt(&curdata);

	rc = 1;
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}

#endif

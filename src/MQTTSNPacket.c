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

static char* packet_names[] =
{
		"ADVERTISE", "SEARCHGW", "GWINFO", "RESERVED", "CONNECT", "CONNACK",
		"WILLTOPICREQ", "WILLTOPIC", "WILLMSGREQ", "WILLMSG", "REGISTER", "REGACK",
		"PUBLISH", "PUBACK", "PUBCOMP", "PUBREC", "PUBREL", "RESERVED",
		"SUBSCRIBE", "SUBACK", "UNSUBSCRIBE", "UNSUBACK", "PINGREQ", "PINGRESP",
		"DISCONNECT", "RESERVED", "WILLTOPICUPD", "WILLTOPICRESP", "WILLMSGUPD",
		"WILLMSGRESP"
};


/**
 * Returns a character string representing the packet name given a MsgType code
 * @param code MsgType code
 * @return the corresponding packet name
 */
char* MQTTSNPacket_name(int code)
{
	return (code >= 0 && code <= MQTTSN_WILLMSGRESP) ? packet_names[code] : "UNKNOWN";
}


/**
 * Calculates the full packet length including length field
 * @param length the length of the MQTT-SN packet without the length field
 * @return the total length of the MQTT-SN packet including the length field
 */
int MQTTSNPacket_len(int length)
{
	return (length > 255) ? length + 3 : length + 1;
}


/**
 * Encodes the MQTT-SN message length
 * @param buf the buffer into which the encoded data is written
 * @param length the length to be encoded
 * @return the number of bytes written to the buffer
 */
int MQTTSNPacket_encode(unsigned char* buf, int length)
{
	int rc = 0;

	FUNC_ENTRY;
	if (length > 255)
	{
		buf[rc++] = 0x01;
		writeInt(&buf, length);
		rc += 2;
	}
	else
		buf[rc++] = length;

	FUNC_EXIT_RC(rc);
	return rc;
}


/**
 * Obtains the MQTT-SN packet length from received data
 * @param getcharfn pointer to function to read the next character from the data source
 * @param value the decoded length returned
 * @return the number of bytes read from the socket
 */
int MQTTSNPacket_decode(unsigned char* buf, size_t buflen, int* value)
{
	int len = MQTTSNPACKET_READ_ERROR;
#define MAX_NO_OF_LENGTH_BYTES 3

	FUNC_ENTRY;
	if (buflen <= 0)
		goto exit;

	if (buf[0] == 1)
	{
		unsigned char* bufptr = &buf[1];
		if (buflen < 3)
			goto exit;
		*value = readInt(&bufptr);
		len = 3;
	}
	else
	{
		*value = buf[0];
		len = 1;
	}
exit:
	FUNC_EXIT_RC(len);
	return len;
}


/**
 * Calculates an integer from two bytes read from the input buffer
 * @param pptr pointer to the input buffer - incremented by the number of bytes used & returned
 * @return the integer value calculated
 */
int readInt(unsigned char** pptr)
{
	unsigned char* ptr = *pptr;
	int len = 256*((unsigned char)(*ptr)) + (unsigned char)(*(ptr+1));
	*pptr += 2;
	return len;
}


/**
 * Reads one character from the input buffer.
 * @param pptr pointer to the input buffer - incremented by the number of bytes used & returned
 * @return the character read
 */
char readChar(unsigned char** pptr)
{
	char c = **pptr;
	(*pptr)++;
	return c;
}


/**
 * Writes one character to an output buffer.
 * @param pptr pointer to the output buffer - incremented by the number of bytes used & returned
 * @param c the character to write
 */
void writeChar(unsigned char** pptr, char c)
{
	**pptr = (unsigned char)c;
	(*pptr)++;
}


/**
 * Writes an integer as 2 bytes to an output buffer.
 * @param pptr pointer to the output buffer - incremented by the number of bytes used & returned
 * @param anInt the integer to write: 0 to 65535
 */
void writeInt(unsigned char** pptr, int anInt)
{
	**pptr = (unsigned char)(anInt / 256);
	(*pptr)++;
	**pptr = (unsigned char)(anInt % 256);
	(*pptr)++;
}


/**
 * Writes a "UTF" string to an output buffer.  Converts C string to length-delimited.
 * @param pptr pointer to the output buffer - incremented by the number of bytes used & returned
 * @param string the C string to write
 */
void writeCString(unsigned char** pptr, char* string)
{
	int len = strlen(string);
	memcpy(*pptr, string, len);
	*pptr += len;
}


int getLenStringLen(char* ptr)
{
	int len = 256*((unsigned char)(*ptr)) + (unsigned char)(*(ptr+1));
	return len;
}


void writeMQTTSNString(unsigned char** pptr, MQTTString mqttstring)
{
	if (mqttstring.lenstring.len > 0)
	{
		memcpy(*pptr, mqttstring.lenstring.data, mqttstring.lenstring.len);
		*pptr += mqttstring.lenstring.len;
	}
	else if (mqttstring.cstring)
		writeCString(pptr, mqttstring.cstring);
}


/**
 * @param mqttstring the MQTTString structure into which the data is to be read
 * @param pptr pointer to the output buffer - incremented by the number of bytes used & returned
 * @param enddata pointer to the end of the data: do not read beyond
 * @return 1 if successful, 0 if not
 */
int readMQTTSNString(MQTTString* mqttstring, unsigned char** pptr, unsigned char* enddata)
{
	int rc = 0;

	FUNC_ENTRY;
	mqttstring->lenstring.len = enddata - *pptr;
	if (mqttstring->lenstring.len > 0)
	{
		mqttstring->lenstring.data = (char*)*pptr;
		*pptr += mqttstring->lenstring.len;
	}
	else
		mqttstring->lenstring.data = NULL;
	mqttstring->cstring = NULL;
	rc = 1;
	FUNC_EXIT_RC(rc);
	return rc;
}


/**
 * Return the length of the MQTTstring - C string if there is one, otherwise the length delimited string
 * @param mqttstring the string to return the length of
 * @return the length of the string
 */
int MQTTstrlen(MQTTString mqttstring)
{
	int rc = 0;

	if (mqttstring.cstring)
		rc = strlen(mqttstring.cstring);
	else
		rc = mqttstring.lenstring.len;
	return rc;
}


/**
 * Helper function to read packet data from some source into a buffer
 * @param buf the buffer into which the packet will be serialized
 * @param buflen the length in bytes of the supplied buffer
 * @param getfn pointer to a function which will read any number of bytes from the needed source
 * @return integer MQTT packet type, or MQTTSNPACKET_READ_ERROR on error
 */
int MQTTSNPacket_read(unsigned char* buf, int buflen, int (*getfn)(unsigned char*, size_t))
{
	int rc = MQTTSNPACKET_READ_ERROR;
	const int MQTTSN_MIN_PACKET_LENGTH = 3;
	int len = 0;  /* the length of the whole packet including length field */
	int lenlen = 0;
	int datalen = 0;

	/* 1. read a packet - UDP style */
	if ((len = (*getfn)(buf, buflen)) < MQTTSN_MIN_PACKET_LENGTH)
		goto exit;

	/* 2. read the length.  This is variable in itself */
	lenlen = MQTTSNPacket_decode(buf, len, &datalen);
	if (datalen != len)
		goto exit; /* there was an error */

	rc = buf[lenlen]; /* return the packet type */
exit:
	return rc;
}



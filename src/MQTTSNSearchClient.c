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

#include "MQTTSNPacket.h"
#include "StackTrace.h"

#include <string.h>


int MQTTSNDeserialize_advertise(unsigned char* gatewayid, unsigned short* duration,	unsigned char* buf, int buflen)
{
	unsigned char* curdata = buf;
	unsigned char* enddata = NULL;
	int rc = 0;
	int mylen = 0;

	FUNC_ENTRY;
	curdata += (rc = MQTTSNPacket_decode(curdata, buflen, &mylen)); /* read length */
	enddata = buf + mylen;
	if (enddata - curdata > buflen)
		goto exit;

	if (readChar(&curdata) != MQTTSN_ADVERTISE)
		goto exit;

	*gatewayid = readChar(&curdata);
	*duration = readInt(&curdata);

	rc = 1;
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


int MQTTSNSerialize_searchgw(unsigned char* buf, int buflen, unsigned char radius)
{
	unsigned char *ptr = buf;
	int len = 0;
	int rc = 0;

	FUNC_ENTRY;
	if ((len = MQTTSNPacket_len(2)) > buflen)
	{
		rc = MQTTSNPACKET_BUFFER_TOO_SHORT;
		goto exit;
	}
	ptr += MQTTSNPacket_encode(ptr, len);   /* write length */
	writeChar(&ptr, MQTTSN_SEARCHGW);      /* write message type */

	writeChar(&ptr, radius);

	rc = ptr - buf;
exit:
	FUNC_EXIT_RC(rc);
	return rc;

}


int MQTTSNDeserialize_gwinfo(unsigned char* gatewayid, unsigned short* gatewayaddress_len,
		unsigned char** gatewayaddress, unsigned char* buf, int buflen)
{
	unsigned char* curdata = buf;
	unsigned char* enddata = NULL;
	int rc = 0;
	int mylen = 0;

	FUNC_ENTRY;
	curdata += (rc = MQTTSNPacket_decode(curdata, buflen, &mylen)); /* read length */
	enddata = buf + mylen;
	if (enddata - curdata > buflen)
		goto exit;

	if (readChar(&curdata) != MQTTSN_GWINFO)
		goto exit;

	*gatewayid = readChar(&curdata);

	*gatewayaddress_len = enddata - curdata;
	*gatewayaddress = curdata;

	rc = 1;
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}






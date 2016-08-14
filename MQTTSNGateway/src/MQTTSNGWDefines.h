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
 *    Tomoaki Yamaguchi - initial API and implementation and/or initial documentation
 **************************************************************************************/

#ifndef MQTTSNGWDEFINES_H_
#define MQTTSNGWDEFINES_H_

namespace MQTTSNGW
{
/*=================================
 *    Log controls
 ==================================*/
//#define DEBUG          // print out log for debug
//#define RINGBUFFER     // print out Packets log into shared memory
//#define DEBUG_NWSTACK  // print out SensorNetwork log

/*=================================
 *    Parametrs
 ==================================*/
#define MAX_CLIENTID_LENGTH          (64)  // Max length of clientID
#define MQTTSNGW_MAX_PACKET_SIZE   (1024)  // Max Packet size  (5+2+TopicLen+PayloadLen)
#define SIZEOF_LOG_PACKET           (500)  // Length of the packet log in bytes

//#define CLIENTS_OTA_AVAILABLE
#define MQTTSNGW_TLS_CA_DIR       "/etc/ssl/certs"

/*=================================
 *    Data Type
 ==================================*/
typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;

/*=================================
 *    Macros
 ==================================*/
#ifdef  DEBUG
#define DEBUGLOG(...) printf(__VA_ARGS__)
#undef  RINGBUFFER
#else
#define DEBUGLOG(...)
#endif

}
#endif /* MQTTSNGWDEFINES_H_ */

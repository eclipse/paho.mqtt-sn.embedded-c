/**************************************************************************************
 * Copyright (c) 2021, Tomoaki Yamaguchi
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
#include "LMqttsnClientApp.h"
#ifdef BLE

#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>

#include "LNetworkBle.h"
#include "LTimer.h"
#include "LScreen.h"

using namespace std;
using namespace linuxAsyncClient;

extern uint16_t getUint16(const uint8_t* pos);
extern uint32_t getUint32(const uint8_t* pos);
extern LScreen* theScreen;
extern bool theClientMode;
extern LBleConfig theNetcon;
/*=========================================
 Class LNetwork
 =========================================*/
LNetwork::LNetwork()
{
    _sleepflg = false;
    _returnCode = 0;
}

LNetwork::~LNetwork()
{

}

int LNetwork::broadcast(const uint8_t* xmitData, uint16_t dataLen)
{
    return LBlePort::unicast(xmitData, dataLen);
}

int LNetwork::unicast(const uint8_t* xmitData, uint16_t dataLen)
{
    return LBlePort::unicast(xmitData, dataLen);
}

uint8_t* LNetwork::getMessage(int* len)
{
    *len = 0;
    if (checkRecvBuf())
    {
        uint16_t recvLen = LBlePort::recv(_rxDataBuf, MQTTSN_MAX_PACKET_SIZE, false);

        if (recvLen < 0)
        {
            *len = recvLen;
            return 0;
        }
        else
        {
            if (_rxDataBuf[0] == 0x01)
            {
                *len = getUint16(_rxDataBuf + 1);
            }
            else
            {
                *len = _rxDataBuf[0];
            }
            //if(recvLen != *len){
            //  *len = 0;
            //  return 0;
            //}else{
            return _rxDataBuf;
            //}
        }
    }
    return 0;
}

void LNetwork::setGwAddress(void)
{
}

void LNetwork::setFixedGwAddress(void)
{
    _channel = LBlePort::_channel;
    memcpy(_gwAddress, theNetcon.gwAddress, 6);
}

bool LNetwork::initialize(LBleConfig config)
{
    return LBlePort::open(config);
}

void LNetwork::setSleep()
{
    _sleepflg = true;
}

bool LNetwork::isBroadcastable()
{
    return false;
}

/*=========================================
 Class BleStack
 =========================================*/
LBlePort::LBlePort()
{
    _disconReq = false;
    _sockBle = 0;
    _channel = 0;
}

LBlePort::~LBlePort()
{
    close();
}

void LBlePort::close()
{
    if (_sockBle > 0)
    {
        ::close(_sockBle);
        _sockBle = 0;
    }
}

bool LBlePort::open(LBleConfig config)
{
    const int reuse = 1;
    uint8_t* gw = config.gwAddress + 5;
    for (int i = 0; i < 6; i++)
    {
        *(_gwAddress + i) = *gw--;
    }
    _channel = config.channel;

    if (_channel == 0 || _gwAddress == 0 || _devAddress == 0)
    {
        D_NWLOG("\033[0m\033[0;31merror BLE Address in BlePort::open\033[0m\033[0;37m\n");
        DISPLAY("\033[0m\033[0;31m\nerror BLE Address in BlePort::open\033[0m\033[0;37m\n");
        return false;
    }

    _sockBle = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
    if (_sockBle < 0)
    {
        D_NWLOG("\033[0m\033[0;31merror Can't create socket in BlePort::open\033[0m\033[0;37m\n");
        DISPLAY("\033[0m\033[0;31m\nerror Can't create socket in BlePort::open\033[0m\033[0;37m\n");
        return false;
    }

    setsockopt(_sockBle, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    struct sockaddr_rc addru = { 0 };
    addru.rc_family = AF_BLUETOOTH;
    addru.rc_channel = _channel;
    memcpy(&addru.rc_bdaddr, _gwAddress, 6);

    char bufgw[30];
    ba2str(&addru.rc_bdaddr, bufgw);
    DISPLAY("GW MAC = %s  RFCOMM CH = %d\n", bufgw, addru.rc_channel);

    // connect to server
    errno = 0;
    int status = connect(_sockBle, (struct sockaddr *) &addru, sizeof(addru));
    if (status < 0)
    {
        D_NWLOG("\033[0m\033[0;31merror = %d Can't connect to GW in BlePort::open\033[0m\033[0;37m\n", errno);
        DISPLAY("\033[0m\033[0;31mCan't connect to GW Ble socket in BlePort::open\033[0m\033[0;37m\n");
        close();
        return false;
    }
    return true;
}

int LBlePort::unicast(const uint8_t* buf, uint32_t length)
{
    int status = ::write(_sockBle, buf, length);
    if (status < 0)
    {
        D_NWLOG("errno == %d in LBlePort::unicast\n", errno);
        DISPLAY("errno == %d in LBlePort::unicast\n", errno);
    }
    else
    {
        D_NWLOG("sendto %-2d", _channel);
        for (uint16_t i = 0; i < length; i++)
        {
            D_NWLOG(" %02x", *(buf + i));
        }
        D_NWLOG("\n");

        if (!theClientMode)
        {
            char sbuf[SCREEN_BUFF_SIZE];
            int pos = 0;
            sprintf(sbuf, "\033[0;34msendto %-2dch", _channel);
            pos = strlen(sbuf);
            for (uint16_t i = 0; i < length; i++)
            {
                sprintf(sbuf + pos, " %02x", *(buf + i));
                if (strlen(sbuf) > SCREEN_BUFF_SIZE - 20)  // -20 for Escape sequence
                {
                    break;
                }
                pos += 3;
            }
            sprintf(sbuf + strlen(sbuf), "\033[0;37m\n");
            theScreen->display(sbuf);
        }
    }
    return status;
}

bool LBlePort::checkRecvBuf()
{
    uint8_t buf[2];
    if (::recv(_sockBle, buf, 1, MSG_DONTWAIT | MSG_PEEK) > 0)
    {
        return true;
    }
    return false;
}

int LBlePort::recv(uint8_t* buf, uint16_t length, bool flg)
{
    int flags = flg ? MSG_DONTWAIT : 0;
    int status = ::recv(_sockBle, buf, length, flags);

    if (status < 0 && errno != EAGAIN)
    {
        D_NWLOG("\033[0m\033[0;31merrno == %d in BlePort::recv \033[0m\033[0;37m\n", errno);
        DISPLAY("\033[0m\033[0;31merrno == %d in BlePort::recv \033[0m\033[0;37m\n", errno);
    }
    else if (status > 0)
    {
        D_NWLOG("\nrecved  ");
        for (uint16_t i = 0; i < status; i++)
        {
            D_NWLOG(" %02x", *(buf + i));
        }
        D_NWLOG("\n");

        if (!theClientMode)
        {
            char sbuf[SCREEN_BUFF_SIZE];
            int pos = 0;
            sprintf(sbuf, "\033[0;34mrecved ");
            pos = strlen(sbuf);
            for (uint16_t i = 0; i < status; i++)
            {
                sprintf(sbuf + pos, " %02x", *(buf + i));
                if (strlen(sbuf) > SCREEN_BUFF_SIZE - 20)
                {
                    break;
                }
                pos += 3;
            }
            sprintf(sbuf + strlen(sbuf), "\033[0;37m\n");
            theScreen->display(sbuf);
        }
        return status;
    }
    else
    {
        return 0;
    }
    return status;
}

#endif


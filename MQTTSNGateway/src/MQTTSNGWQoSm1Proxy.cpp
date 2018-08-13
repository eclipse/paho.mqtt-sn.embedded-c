/**************************************************************************************
 * Copyright (c) 2018, Tomoaki Yamaguchi
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

#include "MQTTSNGWQoSm1Proxy.h"
#include "MQTTSNGateway.h"
#include "SensorNetwork.h"
#include "MQTTSNGWClientList.h"
#include <string>
#include <string.h>


using namespace MQTTSNGW;

/*=====================================
     Class QoSm1Proxy
 =====================================*/
QoSm1Proxy:: QoSm1Proxy(Gateway* gw) : Adapter(gw)
{
    _gateway = gw;
}

QoSm1Proxy::~QoSm1Proxy(void)
{

}


void QoSm1Proxy::initialize(void)
{
    char param[MQTTSNGW_PARAM_MAX];

    if ( _gateway->hasSecureConnection() )
    {
		_isSecure = true;
    }

    if (_gateway->getParam("QoS-1", param) == 0 )
    {
        if (strcasecmp(param, "YES") == 0 )
        {
        	/*  Create QoS-1 Clients */
        	_gateway->getClientList()->setClientList(QOSM1PROXY_TYPE);

            /* initialize Adapter */
			string name = string(_gateway->getGWParams()->gatewayName) + "QoS-1";
            setup(name.c_str(), Atype_QoSm1Proxy);
           _isActive = true;
        }
    }
}


bool QoSm1Proxy::isActive(void)
{
    return _isActive;
}


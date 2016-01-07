
#include "Threading.h"

//#include "MQTTSNUDP.h"
//#include "MQTTEthernet.h"
#include "linux.cpp"
#include "MQTTSNGateway.h"

MQTTSN::Parms parms();


int main()
{
    // set up MQTT-SN network listening
    UDPStack net;
    net.listen(1884);
    
    MQTTSN::Gateway<UDPStack, IPStack, Countdown, Thread, Mutex> gateway = 
    			MQTTSN::Gateway<UDPStack, IPStack, Countdown, Thread, Mutex>(net);
    
    gateway.run(NULL);
    
    return 0;
}

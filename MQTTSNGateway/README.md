**MQTT-SN** requires a MQTT-SN Gateway which acts as a protocol converter to convert **MQTT-SN messages to MQTT messages**. MQTT-SN client (UDP) can not communicate directly with MQTT broker(TCP/IP).   

### **step1. Build the gateway**   
````
$ git clone -b gateway https://github.com/eclipse/paho.mqtt-sn.embedded-c 
$ cd paho.mqtt-sn.embedded-c       
$ make   
$ make install   
$ make clean
```       
MQTT-SNGateway, MQTT-SNLogmonitor and param.conf are copied into ../ directory.

    
### **step2. Execute the Gateway.**     

````    
$ cd ../   
$ ./MQTT-SNGateway
````   
or you can specify the config file as follows:    
```` 
$ ./MQTT-SNGateway -f configFilePath/configFileName    
```` 
        
MQTT-SN clients should run in differnt from a host of the gateway. Other wise the gateway can't receive multicast packets form clients.    

### **How to Change the configuration of the gateway**    
**../param.conf**   Contents are follows: 
   
````
# config file of MQTT-SN Gateway

BrokerName=test.mosquitto.org    
BrokerPortNo=1883    
SecureConnection=NO    
#BrokerPortNo=8883    
#SecureConnection=YES    
#ClientAuthorization=./clientList.conf    
GatewayID=1    
GatewayName=PahoGateway-01    
KeepAlive=900    
#LoginID=    
#Password=    
    
# UDP    
GatewayPortNo=2000    
MulticastIP=225.1.1.1    
MulticastPortNo=1883    
    
# XBee    
Baudrate=38400    
SerialDevice=/dev/ttyUSB0    
```

**BrokerName** to specify a domain name of the Broker, and **BrokerPortNo** is a port No of the Broker. If the Broker have to connected via TLS, set BrokerPortNo=8883 and **SecureConnection=YES**.     
**MulticastIP** and **MulticastPortNo** is a multicast address for ADVERTISE, GWSEARCH and GWINFO messages. Gateway is waiting GWSEARCH multicast message and when receiving it send GWINFO message via Broadcast address. Clients can get the gateway address (Gateway IP address and **GatewayPortNo**) from GWINFO message by means of std::recvfrom(),
Client should know the BroadcastIP and PortNo to send a SEARCHGW message.    
**GatewayId** is defined by GWSEARCH message.    
**KeepAlive** is a duration of ADVERTISE message in seconds.    
when **ClientAuthorization** is clientList file for authorization. see MQTTSNGWClient.cpp line53. this file defines connect able clients by IPaddress and PortNo in case of UDP.    


### ** How to change the config directory. **
Default pram.conf file and key files, rbmutex.key and ringbuffer.key, are created automatically in this directory.    
The directory is defined in MQTTSNGWProcess.h line33.    
change this value to new directory you want to use.
       
`#define MQTTSNGW_CONFIG_DIRECTORY "./"`

### ** How to monitor the gateway from remote. **

Uncomment line32 in MQTTSNGWDefined.h.

`//#define RINGBUFFER     // print out Packets log into shared memory./"`    
````    
$ make   
$ make install 
$ make clean
````
restart the gateway.    
open ssh terminal and execute LogMonitor.

`$ ./MQTT-SNLogmonitor`    
 
`$ ./MQTT-SNLogmonitor`      
Now you can get the Log on your terminal.



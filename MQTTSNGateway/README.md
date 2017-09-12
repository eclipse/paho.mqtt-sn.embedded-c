**MQTT-SN** requires a MQTT-SN Gateway which acts as a protocol converter to convert **MQTT-SN messages to MQTT messages**. MQTT-SN client (UDP) can not communicate directly with MQTT broker(TCP/IP).   

### **step1. Build the gateway**   
````
$ git clone https://github.com/eclipse/paho.mqtt-sn.embedded-c   
$ cd paho.mqtt-sn.embedded-c/MQTTSNGateway       
$ make   
$ make install   
$ make clean    
````      
MQTT-SNGateway, MQTT-SNLogmonitor and param.conf are copied into ../ directory.

    
### **step2. Execute the Gateway.**     

````    
$ cd ../   
$ ./MQTT-SNGateway [-f Config file name]
````   


### **How to Change the configuration of the gateway**    
**../gateway.conf**   Contents are follows: 
   
````    

# config file of MQTT-SN Gateway

BrokerName=iot.eclipse.org
BrokerPortNo=1883
BrokerSecurePortNo=8883    

ClientAuthentication=NO
#ClientsList=/path/to/your_clients.conf    
    
#RootCAfile=/path/to/your_Root_CA.crt    
#RootCApath=/path/to/your_certs_directory/   
#CertKey=/path/to/your_cert.pem
#PrivateKey=/path/to/your_private-key.pem
    
GatewayID=1    
GatewayName=PahoGateway-01    
KeepAlive=900    
#LoginID=your_ID    
#Password=your_Password    

# UDP
GatewayPortNo=10000    
MulticastIP=225.1.1.1    
MulticastPortNo=1883    

# XBee
Baudrate=38400    
SerialDevice=/dev/ttyUSB0    
ApiMode=2    
````    

**BrokerName** to specify a domain name of the Broker, and **BrokerPortNo** is a port No of the Broker. **BrokerSecurePortNo** is for TLS connection.       
**MulticastIP** and **MulticastPortNo** is a multicast address for GWSEARCH messages. Gateway is waiting GWSEARCH  and when receiving it send GWINFO message via MulticastIP address. Clients can get the gateway address (Gateway IP address and **GatewayPortNo**) from GWINFO message by means of std::recvfrom().
Client should know the MulticastIP and MulticastPortNo to send a SEARCHGW message.    
**GatewayId** is used by GWINFO message.    
**KeepAlive** is a duration of ADVERTISE message in seconds.    
when **ClientAuthentication** is YES, see MQTTSNGWClient.cpp line53, clients file specified by ClientsList is required. This file defines connect allowed clients by ClientId and SensorNetwork Address. e.g. IP address and Port No.    



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

Now you can get the Log on your terminal.



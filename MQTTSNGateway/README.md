**MQTT-SN** requires a MQTT-SN Gateway which acts as a protocol converter to convert **MQTT-SN messages to MQTT messages**. MQTT-SN client (UDP) can not communicate directly with MQTT broker(TCP/IP).   

### **step1. Build the gateway**   
````
$ git clone -b gateway https://github.com/eclipse/paho.mqtt-sn.embedded-c       
$ make    
$ make install   
```       
or compile with Eclipse CDT    
  

### **step2. Create configuration file of the gateway**    
Create  **/usr/local/etc/mqttsnGateway/config/param.conf**     
Contents are follows:    
    
````
BrokerName=test.mosquitto.org
BrokerPortNo=1883
SecureConnection=NO
#BrokerPortNo=8883
#SecureConnection=YES
ClientAuthorization=NO
GatewayID=1
GatewayName=PahoGateway-01
KeepAlive=900
#LoginID=
#Password=
BroadcastIP=225.1.1.1
GatewayPortNo=2000
BroadcastPortNo=1883
```

**BrokerName** to specify a domain name of the Broker, and **BrokerPortNo** is a port No of the Broker. If the Broker have to connected via TLS, set BrokerPortNo=8883 and **SecureConnection=YES**.     
**BroadcastIP** and **BroadcastPortNo** is a multicast address for ADVERTISE, GWSEARCH and GWINFO messages. Gateway is waiting GWSEARCH multicast message and when receiving it send GWINFO message via Broadcast address. Clients can get the gateway address (Gateway IP address and **GatewayPortNo**) from GWINFO message by means of std::recvfrom(),
Client should know the BroadcastIP and PortNo to send a SEARCHGW message.    
**GatewayId** is defined by GWSEARCH message.    
**KeepAlive** is a duration of ADVERTISE message in seconds.    
when **ClientAuthorization** is YES, see MQTTSNGWClient.cpp line53, /usr/local/etc/mqttsnGateway/config/clientList.conf file is required. this file defines connect able clients by IPaddress and PortNo.



### **step3. Create key files of the ring buffer** 

Create the following 3 empty files as key files.    
1) /usr/local/etc/mqttsnGateway/config/**rbmutex.key**    
2) /usr/local/etc/mqttsnGateway/config/**ringbuffer.key**    
3) /usr/local/etc/mqttsnGateway/config/**semaphore.key**    
    
### **step4. Execute the Gateway.**     
This must not be the same machine as the Client. 
    
`$ ./MQTT-SNGateway`
    


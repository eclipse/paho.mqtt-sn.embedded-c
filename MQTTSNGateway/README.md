 MQTT-SN Transparent / Aggregating Gateway

**MQTT-SN** requires a MQTT-SN Gateway which acts as a protocol converter to convert **MQTT-SN messages to MQTT messages**. MQTT-SN client over SensorNetwork can not communicate directly with MQTT broker(TCP/IP).   
This Gateway can run as a transparent or aggregating Gateway by specifying the gateway.conf.    

### **step1. Build the gateway**   
````
$ git clone -b develop https://github.com/eclipse/paho.mqtt-sn.embedded-c  
$ cd paho.mqtt-sn.embedded-c/MQTTSNGateway      
$ ./build.sh [udp|udp6|xbee|loralink | ble]  
    
````       
In order to build a gateway, an argument is required.  
 
MQTT-SNGateway and MQTT-SNLogmonitor (executable programs) are built in the Build directory.
    
### **step2. Execute the Gateway.**     

````    
$ cd bin 
$ ./MQTT-SNGateway    
````   
If you get the error message as follows:
````    
RingBuffer can't create a shared memory.
ABORT Gateway!!!    
````
You have to start using sudo command only once for the first time.    
````
$ sudo ./MQTT-SNGateway    
````

### **How to Change the configuration of the gateway**    
**gateway.conf**   Contents are follows: 
   
<pre><dev>
# config file of MQTT-SN Gateway
#

BrokerName=mqtt.eclipseprojects.io
BrokerPortNo=1883
BrokerSecurePortNo=8883

#
# When AggregatingGateway=YES or ClientAuthentication=YES,
# All clients must be specified by the ClientList File  
#

ClientAuthentication=NO
AggregatingGateway=NO
QoS-1=NO
Forwarder=NO
MaxNumberOfClients=30;

#ClientsList=/path/to/your_clients.conf

PredefinedTopic=NO
#PredefinedTopicList=/path/to/your_predefinedTopic.conf

#RootCAfile=/etc/ssl/certs/ca-certificates.crt
#RootCApath=/etc/ssl/certs/
#CertsFile=/path/to/certKey.pem
#PrivateKey=/path/to/privateKey.pem

GatewayID=1
GatewayName=PahoGateway-01
KeepAlive=900
#LoginID=your_ID
#Password=your_Password


# UDP
GatewayPortNo=10000
MulticastIP=225.1.1.1
MulticastPortNo=1883
MulticastTTL=1  

# UDP6
GatewayUDP6Bind=FFFF:FFFE::1 
GatewayUDP6Port=10000
GatewayUDP6Broadcast=FF02::1
GatewayUDP6If=wpan0
GatewayUDP6Hops=1

# XBee
Baudrate=38400
SerialDevice=/dev/ttyUSB0
ApiMode=2

#LoRaLink
BaudrateLoRaLink=115200
DeviceRxLoRaLink=/dev/ttyLoRaLinkRx
DeviceTxLoRaLink=/dev/ttyLoRaLinkTx

# BLE RFCOMM
BleAddress=60:57:18:06:8B:72.*

# LOG
ShearedMemory=NO;

</dev></pre>

**BrokerName** to specify a domain name of the Broker, and **BrokerPortNo** is a port No of the Broker. **BrokerSecurePortNo** is for TLS connection.       
**MulticastIP** and **MulticastPortNo** is a multicast address for GWSEARCH messages. Gateway is waiting GWSEARCH  and when receiving it send GWINFO message via MulticastIP address. Clients can get the gateway address (Gateway IP address and **GatewayPortNo**) from GWINFO message by means of std::recvfrom().
Client should know the MulticastIP and MulticastPortNo to send a SEARCHGW message.    
**GatewayId** is used by GWINFO message.    
**KeepAlive** is a duration of ADVERTISE message in seconds.    
when **AggregatingGateway** or **ClientAuthentication** is **YES**, All clients which connect to the gateway must be declared by a **ClientsList** file.       
Format of the file is ClientId and SensorNetwork Address. e.g. IP address and Port No etc, in CSV. more detail see clients.conf.    
When **QoS-1** is **YES**, QoS-1 PUBLISH is available. All clients which send QoS-1 PUBLISH must be specified by Client.conf file. 
When **PredefinedTopic** is **YES**, **Pre-definedTopicId**s  specified by **PredefinedTopicList** are effective. This file defines Pre-definedTopics of the clients. In this file, ClientID,TopicName and TopicID are declared in CSV format.    
When **Forwarder** is **YES**, Forwarder Encapsulation Message is available. Connectable Forwarders must be declared by a **ClientsList** file.     
**MaxNumberOfClients** Maximum number of clients allocated.
 
### ** How to monitor the gateway from remote. **
Change gateway.conf as follows:
```
# LOG
ShearedMemory=YES;
````

Restart the gateway with sudo only once to create shared memories.    

open ssh terminal and execute LogMonitor.
 
```
 $ cd bin
 $ ./MQTT-SNLogmonitor
```  

Now you can get the Log on your terminal.


## ** Tips **
Uncomment the line 62, 63 in MQTTSNDefines.h then you can get more precise logs.
```
/*=================================
 *    Log controls
 ==================================*/
//#define DEBUG          // print out log for debug
//#define DEBUG_NWSTACK  // print out SensorNetwork log
```
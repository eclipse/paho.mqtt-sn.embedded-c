# MQTT-SN Transparent / Aggregating Gateway

**MQTT-SN** requires a MQTT-SN Gateway which acts as a protocol converter to convert **MQTT-SN messages to MQTT messages**. MQTT-SN client over SensorNetwork can not communicate directly with MQTT broker (TCP/IP).
This Gateway can be configured to run as a transparent or aggregating gateway in the file *gateway.conf*.

### **Step 1: Build the gateway**
````
$ git clone https://github.com/eclipse/paho.mqtt-sn.embedded-c
$ cd paho.mqtt-sn.embedded-c/MQTTSNGateway
$ make
$ make install
$ make clean
````
MQTT-SNGateway, MQTT-SNLogmonitor and *.conf files are copied into **../** directory.
If you want to install the gateway into specific directories, enter a command line as follows:
````
$ make install INSTALL_DIR=/path/to/your_directory CONFIG_DIR=/path/to/your_directory
````


### **Step 2: Execute the Gateway.**

````
$ cd ../
$ ./MQTT-SNGateway [-f Config file name]
````


### How to change the configuration of the gateway
Example for **gateway.conf**:

<pre><dev>

# config file of MQTT-SN Gateway
#

BrokerName=iot.eclipse.org
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

# XBee
Baudrate=38400
SerialDevice=/dev/ttyUSB0
ApiMode=2

# LOG
ShearedMemory=NO;

</dev></pre>

**Broker config**
* *BrokerName* - Domain name/ IP address of the MQTT broker
* *BrokerPortNo* - Port of the MQTT broker
* *LoginID* - Username for login at the broker
* *Password* - Password for login at the broker

**Broker config for TLS**
* *BrokerSecurePortNo* - TLS Port of the MQTT broker
* *RootCAfile* - Path to the root CA file
* *RootCApath* - Path to the CA certificates
* *CertsFile* - Path to the certificate
* *PrivateKey* - Path to the private key

**Gateway config**
* *MulticastIP* - UDP multicast IP address of the gateway
* *MulticastPortNo* - UDP multicast port of the gateway
* *GatewayId* - ID of the gateway (for advertising)
* *GatewayName* - Name of the gateway (for advertising)
* *KeepAlive* - Connection timeout
* *AggregatingGateway* - If 'YES', all clients which want to connect to the gateway must be declared inside of *clients.conf*
* *ClientAuthentication* - If 'YES', all clients which want to connect to the gateway must be declared inside of *clients.conf*
* *ClientsList* - path/to/your/custom_clients.conf
* *PredefinedTopic* - If 'YES', then predefined topics specified by *predefinedTopic.conf* are effective
* *PredefinedTopicList* - path/to/your/custom_predefinedTopic.conf
* *QoS-1* - If 'YES', QoS-1 PUBLISH is available. All clients which send QoS-1 PUBLISH must be declared inside of *clients.conf*
* *Forwarder* - If 'YES', then Forwarder Encapsulation Message is available. Connectable forwarders must be declared in *clients.conf*.

Multicast address is used for GWSEARCH messages. The Gateway is waiting GWSEARCH  and when receiving it,
it sends GWINFO message via MulticastIP address. Clients can get the gateway address (Gateway IP address
and GatewayPortNo) from GWINFO message by means of std::recvfrom(). Client should know the MulticastIP and
MulticastPortNo to send a SEARCHGW message.

### How to monitor the gateway from remote

Uncomment line32 in MQTTSNGWDefined.h:

`//#define RINGBUFFER     // print out Packets log into shared memory.`    
````    
$ make   
$ make install 
$ make clean
````
restart the gateway.    
open ssh terminal and execute LogMonitor.

````
$ ./MQTT-SNLogmonitor
````    

Now you can get the Log on your terminal.


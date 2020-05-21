# MQTT-SN Transparent / Aggrigating Gateway

**MQTT-SN** requires a MQTT-SN Gateway which acts as a protocol converter to convert **MQTT-SN messages to MQTT messages**. MQTT-SN client over SensorNetwork can not communicate directly with MQTT broker(TCP/IP).
This Gateway can run as a transparent or aggrigating Gateway by specifying the gateway.conf.

## **Step1 - Build Gateway**

````bash
$git clone https://github.com/eclipse/paho.mqtt-sn.embedded-c
$cd paho.mqtt-sn.embedded-c/MQTTSNGateway
$make
$make install
$make clean
````

By default, a gateway for UDP is built.
In order to create a gateway for UDP6 or XBee, SENSORNET argument is required.

```bash
$make [SENSORNET={udp6|xbee}]
```

MQTT-SNGateway, MQTT-SNLogmonitor and *.conf files are copied into ../../ directory.

If you want to install the gateway into specific directories, enter a command line as follows:

````bash
make install INSTALL_DIR=/path/to/your_directory CONFIG_DIR=/path/to/your_directory
````

## **Step2 - Execute Gateway**

````bash
$cd ../../
$./MQTT-SNGateway [-f Config file name]
````

## **How to Change the configuration of the gateway**

**gateway.conf**   Contents are follows:

```conf
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

# ClientsList=/path/to/your_clients.conf

PredefinedTopic=NO
# PredefinedTopicList=/path/to/your_predefinedTopic.conf

# RootCAfile=/etc/ssl/certs/ca-certificates.crt
# RootCApath=/etc/ssl/certs/
# CertKey=/path/to/certKey.pem
# PrivateKey=/path/to/privateKey.pem

GatewayID=1
GatewayName=PahoGateway-01
KeepAlive=900
# LoginID=your_ID
# Password=your_Password

# UDP
GatewayPortNo=1883
MulticastIP=225.1.1.1
MulticastPortNo=10000

# XBee
Baudrate=38400
SerialDevice=/dev/ttyUSB0
ApiMode=2

# LOG
ShearedMemory=NO
```

* **BrokerName** to specify a domain name or IP address of the Broker.

* **BrokerPortNo** is a port No of the Broker.

* **BrokerSecurePortNo** is port for secure broker TLS connection.

* When **AggregatingGateway** or **ClientAuthentication** is **YES**, All clients which connect to the gateway must be declared by a **ClientsList** file.
Format of the file is ClientId and SensorNetwork Address. e.g. IP address and Port No etc, in CSV. more detail see clients.conf.

* When **QoS-1** is **YES**, QoS-1 PUBLISH is available. All clients which send QoS-1 PUBLISH must be specified by clients.conf file.

* When **Forwarder** is **YES**, Forwarder Encapsulation Message is available. Connectable Forwarders must be declared by a **ClientsList** file.

* When **PredefinedTopic** is **YES**, **Pre-definedTopicId**s  specified by **PredefinedTopicList** are effective. This file defines Pre-definedTopics of the clients. In this file, ClientID,TopicName and TopicID are declared in CSV format.

* **GatewayId** is used by GWINFO message.

* **KeepAlive** is a duration of ADVERTISE message in seconds. When no packet is sent by a client, the client sends a PINGREQ heartbeat packet to the server according to the keepalive period.

* When **LoginID** and **Password** are specified, MQTT-SN Gateway connects to broker securely using these credentials.

* When **RootCAfile**, **CertKey** and **PrivateKey** are specified, MQTT-SN Gateway connects to broker securely using this certificate.

* **MulticastIP** and **MulticastPortNo** is a multicast address for GWSEARCH messages. Gateway is waiting GWSEARCH  and when receiving it send GWINFO message via MulticastIP address. Clients can get the gateway address (Gateway IP address and **GatewayPortNo**) from GWINFO message by means of std::recvfrom().
Client should know the MulticastIP and MulticastPortNo to send a SEARCHGW message.

### **How to monitor the gateway from remote.**

Change gateway.conf as follows:

```conf
# LOG
ShearedMemory=YES
````

Restart the gateway with sudo only once to create shared memories.

open ssh terminal and execute LogMonitor.

`$ ./MQTT-SNLogmonitor`

Now you can get the Log on your terminal.

## **Tips**

Uncomment the following lines in MQTTSNGateway/src/MQTTSNGWDefines.h and rebuild the binaries to get more detailed logs.

```c
/*=================================
 *    Log controls
 ==================================*/
//#define DEBUG          // print out log for debug
//#define DEBUG_NWSTACK  // print out SensorNetwork log
```

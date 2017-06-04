**MQTT-SN** requires a MQTT-SN Gateway which acts as a protocol converter to convert **MQTT-SN messages to MQTT messages**. MQTT-SN client (UDP) can not communicate directly with MQTT broker(TCP/IP).

## Installation

### **step1. Build the gateway**   
```
$ git clone -b gateway https://github.com/eclipse/paho.mqtt-sn.embedded-c 
$ cd paho.mqtt-sn.embedded-c/MQTTSNGateway
$ make
$ make install
$ make clean
```

MQTT-SNGateway, MQTT-SNLogmonitor and param.conf are copied into ../ directory.
You can select the MQTT-SN interface via the Makefile's 'SENSORNET := <your sensornet>' directive.
Following options are currently available:

* **udp** Use an UDPv4 socket based sensor net.
* **udp6** Use an UDPv6 socket based sensor net.
* **xbee** Use a serial XBee bridge to a ZigBee network

    
### **step2. Execute the Gateway.**     

```
$ cd ../
$ ./MQTT-SNGateway [-f Config file name]
```


## **How to Change the configuration of the gateway**    

**../gateway.conf**   Contents are follows: 


```
# config file of MQTT-SN Gateway

#BrokerName=localhost
BrokerName=iot.eclipse.org
BrokerPortNo=1883
BrokerSecurePortNo=8883

ClientAuthentication=NO
#ClientsList=/path/to/your_clients.conf

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

# UDP6
GatewayUDP6Port = 1883
#GatewayUDP6Bind = ff02::1
GatewayUDP6Broadcast = ff02::1
GatewayUDP6If = bt0


# XBee
Baudrate=38400
SerialDevice=/dev/ttyUSB0
ApiMode=2

# LOG
ShearedMemory=NO;
```

### General configuration

* **BrokerName** to specify a host name of the Broker
* and **BrokerPortNo** is the port number of the Broker (unencrypted). **BrokerSecurePortNo** is the port number, used for TLS connection.
* when **ClientAuthentication** is YES, see MQTTSNGWClient.cpp line53, clients file specified by ClientsList is required. This file defines connect allowed clients by ClientId and SensorNetwork Address. e.g. IP address and Port No.
* If TLS and Client/Server authentication is used, define the paths to certificates/keys via **RootCAfile**, **RootCApath**, **CertsFile** and **PrivateKey**.
* **GatewayId** and **GatewayName** is defined for GWSEARCH messages.
* **KeepAlive** is a duration of ADVERTISE message in seconds.
* If necessary, define **LoginID** and **Password** for your MQTT broker


### UDP 

* **MulticastIP** and **MulticastPortNo** is the multicast address for ADVERTISE, GWSEARCH and GWINFO messages. The Gateway is waiting for GWSEARCH multicast message and when receiving it, a GWINFO message is sent via the Broadcast address. Clients can get the gateway address (Gateway IP address and **GatewayPortNo**) from GWINFO message by means of std::recvfrom(),
Clients should know the BroadcastIP and PortNo to send a SEARCHGW message.


### UDPv6

* **GatewayUDP6Port** defines the input port for MQTT-SN messages.
* **GatewayUDP6Bind** can be used to bind to a specific address. If not defined, the first available interface will be used.
* **GatewayUDP6Broadcast** defines a broadcast address, which will be used. Take care of IPv6 broadcasting flags.
* **GatewayUDP6If** defines an interface name which will be used to bind sockets to (for example, different 6loWPAN or BLE networks).

### XBee

* **Baudrate** defines the baudrate for the XBee module.
* **SerialDevice** defines the serial device name.

### ** How to monitor the gateway from remote. **

Uncomment line32 in MQTTSNGWDefined.h.

`//#define RINGBUFFER     // print out Packets log into shared memory./"`

```
$ make
$ make install 
$ make clean
```
restart the gateway.
open ssh terminal and execute LogMonitor.

`$ ./MQTT-SNLogmonitor`

Now you can get the Log on your terminal.

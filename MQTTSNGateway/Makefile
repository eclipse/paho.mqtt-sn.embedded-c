PROGNAME := MQTT-SNGateway
APPL := mainGateway

LPROGNAME := MQTT-SNLogmonitor
LAPPL := mainLogmonitor

TESTPROGNAME := testPFW
TESTAPPL := mainTestProcess

CONFIG := gateway.conf
CLIENTS := clients.conf
PREDEFTOPIC := predefinedTopic.conf

SRCDIR := src
SUBDIR := ../MQTTSNPacket/src

OS := linux
SENSORNET := udp
TEST := tests

INSTALL_DIR=../../
CONFIG_DIR=../../

CPPSRCS :=  \
$(SRCDIR)/MQTTGWConnectionHandler.cpp \
$(SRCDIR)/MQTTGWPacket.cpp \
$(SRCDIR)/MQTTGWPublishHandler.cpp \
$(SRCDIR)/MQTTGWSubscribeHandler.cpp \
$(SRCDIR)/MQTTSNGateway.cpp \
$(SRCDIR)/MQTTSNGWBrokerRecvTask.cpp \
$(SRCDIR)/MQTTSNGWBrokerSendTask.cpp \
$(SRCDIR)/MQTTSNGWClient.cpp \
$(SRCDIR)/MQTTSNGWClientRecvTask.cpp \
$(SRCDIR)/MQTTSNGWClientSendTask.cpp \
$(SRCDIR)/MQTTSNGWConnectionHandler.cpp \
$(SRCDIR)/MQTTSNGWLogmonitor.cpp \
$(SRCDIR)/MQTTSNGWPacket.cpp \
$(SRCDIR)/MQTTSNGWPacketHandleTask.cpp \
$(SRCDIR)/MQTTSNGWProcess.cpp \
$(SRCDIR)/MQTTSNGWPublishHandler.cpp \
$(SRCDIR)/MQTTSNGWSubscribeHandler.cpp \
$(SRCDIR)/MQTTSNGWEncapsulatedPacket.cpp \
$(SRCDIR)/MQTTSNGWForwarder.cpp \
$(SRCDIR)/MQTTSNGWQoSm1Proxy.cpp \
$(SRCDIR)/MQTTSNGWAdapter.cpp \
$(SRCDIR)/MQTTSNGWAggregater.cpp \
$(SRCDIR)/MQTTSNGWClientList.cpp \
$(SRCDIR)/MQTTSNGWTopic.cpp \
$(SRCDIR)/MQTTSNGWAdapterManager.cpp \
$(SRCDIR)/MQTTSNAggregateConnectionHandler.cpp \
$(SRCDIR)/MQTTSNGWMessageIdTable.cpp \
$(SRCDIR)/MQTTSNGWAggregateTopicTable.cpp \
$(SRCDIR)/$(OS)/$(SENSORNET)/SensorNetwork.cpp \
$(SRCDIR)/$(OS)/Timer.cpp  \
$(SRCDIR)/$(OS)/Network.cpp \
$(SRCDIR)/$(OS)/Threading.cpp \
$(SRCDIR)/$(TEST)/TestProcess.cpp \
$(SRCDIR)/$(TEST)/TestQue.cpp \
$(SRCDIR)/$(TEST)/TestTree23.cpp \
$(SRCDIR)/$(TEST)/TestTopics.cpp \
$(SRCDIR)/$(TEST)/TestTopicIdMap.cpp \
$(SRCDIR)/$(TEST)/TestTask.cpp


CSRCS := $(SUBDIR)/MQTTSNConnectClient.c \
$(SUBDIR)/MQTTSNConnectServer.c \
$(SUBDIR)/MQTTSNDeserializePublish.c \
$(SUBDIR)/MQTTSNPacket.c \
$(SUBDIR)/MQTTSNSearchClient.c \
$(SUBDIR)/MQTTSNSearchServer.c \
$(SUBDIR)/MQTTSNSerializePublish.c \
$(SUBDIR)/MQTTSNSubscribeClient.c \
$(SUBDIR)/MQTTSNSubscribeServer.c \
$(SUBDIR)/MQTTSNUnsubscribeClient.c \
$(SUBDIR)/MQTTSNUnsubscribeServer.c 

CXX := g++
CPPFLAGS += 

INCLUDE :=
INCLUDES += $(INCLUDE) -I$(SRCDIR) \
-I$(SRCDIR)/$(OS) \
-I$(SRCDIR)/$(OS)/$(SENSORNET) \
-I$(SUBDIR) \
-I$(SRCDIR)/$(TEST)

DEFS :=
LIB :=
LIBS += $(LIB) -L/usr/local/lib
LDFLAGS := 
CXXFLAGS := -Wall -O3 -std=c++11
LDADD := -lpthread -lssl -lcrypto -lrt
OUTDIR := Build

PROG := $(OUTDIR)/$(PROGNAME)
LPROG := $(OUTDIR)/$(LPROGNAME)
TPROG := $(OUTDIR)/$(TESTPROGNAME)

OBJS := $(CPPSRCS:%.cpp=$(OUTDIR)/%.o)
OBJS += $(CSRCS:%.c=$(OUTDIR)/%.o) 
DEPS := $(CPPSRCS:%.cpp=$(OUTDIR)/%.d)
DEPS += $(CSRCS:%.c=$(OUTDIR)/%.d)

.PHONY: install clean exectest

all: $(PROG) $(LPROG) $(TPROG)

monitor: $(LPROG)

test: $(TPROG) $(LPROG) exectest
	

-include $(DEPS)

$(PROG): $(OBJS) $(OUTDIR)/$(SRCDIR)/$(APPL).o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS) $(LDADD)

$(LPROG): $(OBJS) $(OUTDIR)/$(SRCDIR)/$(LAPPL).o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS) $(LDADD)

$(TPROG): $(OBJS) $(OUTDIR)/$(SRCDIR)/$(TEST)/$(TESTAPPL).o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS) $(LDADD)


$(OUTDIR)/$(SRCDIR)/%.o:$(SRCDIR)/%.cpp
	@if [ ! -e `dirname $@` ]; then mkdir -p `dirname $@`; fi
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(INCLUDES) $(DEFS) -o $@ -c -MMD -MP -MF $(@:%.o=%.d) $<

$(OUTDIR)/$(SRCDIR)/$(APPL).o:$(SRCDIR)/$(APPL).cpp
	@if [ ! -e `dirname $@` ]; then mkdir -p `dirname $@`; fi
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(INCLUDES) $(DEFS) -o $@ -c -MMD -MP -MF $(@:%.o=%.d) $<

$(OUTDIR)/$(SRCDIR)/$(TEST)/$(TESTAPPL).o:$(SRCDIR)/$(TEST)/$(TESTAPPL).cpp
	@if [ ! -e `dirname $@` ]; then mkdir -p `dirname $@`; fi
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(INCLUDES) $(DEFS) -o $@ -c -MMD -MP -MF $(@:%.o=%.d) $<

$(OUTDIR)/$(SRCDIR)/$(LAPPL).o:$(SRCDIR)/$(LAPPL).cpp
	@if [ ! -e `dirname $@` ]; then mkdir -p `dirname $@`; fi
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(INCLUDES) $(DEFS) -o $@ -c -MMD -MP -MF $(@:%.o=%.d) $<

$(OUTDIR)/$(SUBDIR)/%.o:$(SUBDIR)/%.c
	@if [ ! -e `dirname $@` ]; then mkdir -p `dirname $@`; fi
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(INCLUDES) $(DEFS) -o $@ -c -MMD -MP -MF $(@:%.o=%.d) $<

clean:
	rm -rf $(OUTDIR)

install:
	cp -pf $(PROG) $(INSTALL_DIR)
	cp -pf $(LPROG) $(INSTALL_DIR)
	cp -pf $(CONFIG) $(CONFIG_DIR)
	cp -pf $(CLIENTS) $(CONFIG_DIR)
	cp -pf $(PREDEFTOPIC) $(CONFIG_DIR)
	

exectest:
	./$(OUTDIR)/$(TESTPROGNAME) -f ./gateway.conf

	
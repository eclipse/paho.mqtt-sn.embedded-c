PROGNAME := MQTT-SNGateway
APPL := mainGateway

LPROGNAME := MQTT-SNLogmonitor
LAPPL := mainLogmonitor

SRCDIR := MQTTSNGateway/src
SUBDIR := MQTTSNPacket/src

OS := linux
SENSORNET := udp

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
$(SRCDIR)/$(OS)/$(SENSORNET)/SensorNetwork.cpp \
$(SRCDIR)/$(OS)/Timer.cpp  \
$(SRCDIR)/$(OS)/Network.cpp \
$(SRCDIR)/$(OS)/Threading.cpp 

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

INCLUDES += -IMQTTSNGateway/src \
-IMQTTSNGateway/src/$(OS) \
-IMQTTSNGateway/src/$(OS)/$(SENSORNET) \
-IMQTTSNPacket/src

DEFS :=
LIBS +=
LDFLAGS := 
CXXFLAGS := -Wall -O3 -std=c++11
LDADD := -lpthread -lssl -lcrypto
OUTDIR := Build

PROG := $(OUTDIR)/$(PROGNAME)
LPROG := $(OUTDIR)/$(LPROGNAME)
OBJS := $(CPPSRCS:%.cpp=$(OUTDIR)/%.o)
OBJS += $(CSRCS:%.c=$(OUTDIR)/%.o) 
DEPS := $(CPPSRCS:%.cpp=$(OUTDIR)/%.d)
DEPS += $(CSRCS:%.c=$(OUTDIR)/%.d)

.PHONY: install clean 

all: $(PROG) 

monitor: $(LPROG)

-include $(DEPS)

$(PROG): $(OBJS) $(OUTDIR)/$(SRCDIR)/$(APPL).o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS) $(LDADD)

$(LPROG): $(OBJS) $(OUTDIR)/$(SRCDIR)/$(LAPPL).o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS) $(LDADD)

$(OUTDIR)/$(SRCDIR)/%.o:$(SRCDIR)/%.cpp
	@if [ ! -e `dirname $@` ]; then mkdir -p `dirname $@`; fi
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(INCLUDES) $(DEFS) -o $@ -c -MMD -MP -MF $(@:%.o=%.d) $<

$(OUTDIR)/$(SRCDIR)/$(APPL).o:$(SRCDIR)/$(APPL).cpp
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
	cp -pf $(PROG) ../
	
	
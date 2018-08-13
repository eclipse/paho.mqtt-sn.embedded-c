PROGTEST := MQTT-SNGatewayTester
TESTAPPL := mainTest

PRGPUB := MQTT-SNPub
PUBAPPL := mainPub

PRGSUB := MQTT-SNSub
SUBAPPL := mainSub

PRGQOS := MQTT-SNPubQoS-1
QOSAPPL := mainPubQoS-1

SRCDIR := samples
SRCPUB := ClientPub
SRCSUB := ClientSub
SRCQOS := ClientPubQoS-1
SUBDIR := src

CPPSRCS :=  \
$(SUBDIR)/LGwProxy.cpp \
$(SUBDIR)/LMqttsnClient.cpp \
$(SUBDIR)/LNetworkUdp.cpp \
$(SUBDIR)/LPublishManager.cpp \
$(SUBDIR)/LRegisterManager.cpp \
$(SUBDIR)/LSubscribeManager.cpp \
$(SUBDIR)/LTaskManager.cpp \
$(SUBDIR)/LTimer.cpp \
$(SUBDIR)/LTopicTable.cpp \
$(SUBDIR)/LScreen.cpp \
$(SUBDIR)/Payload.cpp \
$(SUBDIR)/Util.cpp \
#$(SRCDIR)/$(TESTAPPL).cpp \
#$(SRCDIR)/$(SRCPUB)/$(PUBAPPL).cpp \
#$(SRCDIR)/$(SRCSUB)/$(SUBAPPL).cpp \

CXX := g++ 
CPPFLAGS += 

INCLUDES += -I$(SUBDIR)
DEFS :=
LIBS +=
LDFLAGS := 
CXXFLAGS := -Wall -O3 -std=c++11
LDADD := 
OUTDIR := Build

PROG := $(OUTDIR)/$(PROGTEST)
OBJS := $(CPPSRCS:%.cpp=$(OUTDIR)/%.o)
DEPS := $(CPPSRCS:%.cpp=$(OUTDIR)/%.d)

PROGPUB := $(OUTDIR)/$(PRGPUB)
PROGSUB := $(OUTDIR)/$(PRGSUB)
PROGQOS := $(OUTDIR)/$(PRGQOS)

.PHONY: install clean 

all: $(PROG) $(PROGPUB) $(PROGSUB) $(PROGQOS)



-include $(DEPS)

$(PROG): $(OBJS) $(OUTDIR)/$(SRCDIR)/$(TESTAPPL).o
	$(CXX) $(LDFLAGS) -o $(PROG) $(OUTDIR)/$(SRCDIR)/$(TESTAPPL).o $(OBJS) $(LIBS) $(LDADD)

$(PROGPUB): $(OBJS) $(OUTDIR)/$(SRCDIR)/$(SRCPUB)/$(PUBAPPL).o
	$(CXX) $(LDFLAGS) -o $(PROGPUB) $(OUTDIR)/$(SRCDIR)/$(SRCPUB)/$(PUBAPPL).o $(OBJS) $(LIBS) $(LDADD)

$(PROGSUB): $(OBJS) $(OUTDIR)/$(SRCDIR)/$(SRCSUB)/$(SUBAPPL).o
	$(CXX) $(LDFLAGS) -o $(PROGSUB) $(OUTDIR)/$(SRCDIR)/$(SRCSUB)/$(SUBAPPL).o $(OBJS) $(LIBS) $(LDADD)

$(PROGQOS): $(OBJS) $(OUTDIR)/$(SRCDIR)/$(SRCQOS)/$(QOSAPPL).o
	$(CXX) $(LDFLAGS) -o $(PROGQOS) $(OUTDIR)/$(SRCDIR)/$(SRCQOS)/$(QOSAPPL).o $(OBJS) $(LIBS) $(LDADD)


$(OUTDIR)/$(SUBDIR)/%.o:$(SUBDIR)/%.cpp
	@if [ ! -e `dirname $@` ]; then mkdir -p `dirname $@`; fi
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(INCLUDES) $(DEFS) -o $@ -c -MMD -MP -MF $(@:%.o=%.d) $<

$(OUTDIR)/$(SRCDIR)/%.o:$(SRCDIR)/%.cpp
	@if [ ! -e `dirname $@` ]; then mkdir -p `dirname $@`; fi
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(INCLUDES) $(DEFS) -o $@ -c -MMD -MP -MF $(@:%.o=%.d) $<

$(OUTDIR)/$(SRCDIR)/$(SRCPUB)/%.o:$(SRCDIR)/$(SRCPUB)%.cpp
	@if [ ! -e `dirname $@` ]; then mkdir -p `dirname $@`; fi
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(INCLUDES) $(DEFS) -o $@ -c -MMD -MP -MF $(@:%.o=%.d) $<

$(OUTDIR)/$(SRCDIR)/$(SRCSUB)/%.o:$(SRCDIR)/$(SRCSUB)%.cpp
	@if [ ! -e `dirname $@` ]; then mkdir -p `dirname $@`; fi
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(INCLUDES) $(DEFS) -o $@ -c -MMD -MP -MF $(@:%.o=%.d) $<
	
$(OUTDIR)/$(SRCDIR)/$(SRCQOS)/%.o:$(SRCDIR)/$(SRCQOS)%.cpp
	@if [ ! -e `dirname $@` ]; then mkdir -p `dirname $@`; fi
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(INCLUDES) $(DEFS) -o $@ -c -MMD -MP -MF $(@:%.o=%.d) $<

clean:
	rm -rf $(OUTDIR)

install:
	cp -pf $(PROG) ../../../
	cp -pf $(PROGPUB) ../../../
	cp -pf $(PROGSUB) ../../../	
	cp -pf $(PROGQOS) ../../../	
	
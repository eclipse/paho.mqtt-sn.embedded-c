PROGNAME := MQTT-SNGatewayTester
APPL := mainTest


SRCDIR := samples
SUBDIR := src

CPPSRCS :=  \
$(SRCDIR)/$(APPL).cpp \
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


CXX := g++ 
CPPFLAGS += 

INCLUDES += -I$(SUBDIR)
DEFS :=
LIBS +=
LDFLAGS := 
CXXFLAGS := -Wall -O3 -std=c++11
LDADD := 
OUTDIR := Build

PROG := $(OUTDIR)/$(PROGNAME)
OBJS := $(CPPSRCS:%.cpp=$(OUTDIR)/%.o)
DEPS := $(CPPSRCS:%.cpp=$(OUTDIR)/%.d)


.PHONY: install clean 

all: $(PROG) 



-include $(DEPS)

$(PROG): $(OBJS) $(OUTDIR)/$(SRCDIR)/$(APPL).o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS) $(LDADD)


$(OUTDIR)/$(SUBDIR)/%.o:$(SUBDIR)/%.cpp
	@if [ ! -e `dirname $@` ]; then mkdir -p `dirname $@`; fi
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(INCLUDES) $(DEFS) -o $@ -c -MMD -MP -MF $(@:%.o=%.d) $<
	
$(OUTDIR)/$(SRCDIR)/%.o:$(SRCDIR)/%.cpp
	@if [ ! -e `dirname $@` ]; then mkdir -p `dirname $@`; fi
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(INCLUDES) $(DEFS) -o $@ -c -MMD -MP -MF $(@:%.o=%.d) $<



clean:
	rm -rf $(OUTDIR)

install:
	cp -pf $(PROG) ../../../

	
	

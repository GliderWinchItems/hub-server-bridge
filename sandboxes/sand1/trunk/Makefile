#
# Makefile.  Generated from Makefile.in by configure.
#
srcdir = .

HUBDIR=../../../trunk
BRIDGEDIR=../../../../bridging/PC

sourcefiles  = $(srcdir)/$(HUBDIR)/hub-server.c 
sourcefiles += $(HUBDIR)/hub-server-sock.c 
sourcefiles += $(HUBDIR)/hub-server-queue.c 
sourcefiles += $(HUBDIR)/hub-server-util.c 
sourcefiles += $(BRIDGEDIR)/can-bridge-filter.c 
sourcefiles += $(BRIDGEDIR)/can-bridge-filter-lookup.c 
sourcefiles += $(BRIDGEDIR)/CANid-hex-bin.c 
sourcefiles += $(srcdir)/sand1.c 

executable = hub-server-bridge-$(SANDBOX)

C_INCLUDES =  -I$(HUBDIR)
C_INCLUDES += -I$(BRIDGEDIR)

CFLAGS =  -g -Wall -Wno-parentheses -DPF_CAN=29 -DAF_CAN=PF_CAN

LIBS = -lpthread 
CC = gcc
LDFLAGS = 
DEFS = -DHAVE_CONFIG_H
CPPFLAGS = 

all: hub-server-bridge-$(SANDBOX)

hub-server-bridge-$(SANDBOX): $(sourcefiles)
	$(CC) $(CFLAGS) $(DEFS) $(CPPFLAGS) $(LDFLAGS) -I . -I ./include $(C_INCLUDES) -o $(executable) $(sourcefiles) $(LIBS)

clean:
	rm -f $(executable) *.o


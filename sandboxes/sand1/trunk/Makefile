
# optimization
OPT = -Og

#######################################
# paths
#######################################
# Build path
BUILD_DIR = build

######################################
# source
######################################
# C sources
sourcefiles = $(srcdir)/can-server.c \
	$(srcdir)/state_raw.c \
	$(srcdir)/can-os.c \
	$(srcdir)/can-so.c \
	$(srcdir)/extract-line.c 

executable = hub-server-bridge-$(SANDBOX)

# /* USER CODE END */ 


# ASM sources
ASM_SOURCES =  \

#startup_stm32f446xx.s

#######################################
# binaries
#######################################
CC = gcc
AS = gcc -x assembler-with-cpp
CP = objcopy
SZ = size

HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S

C_SOURCES = main.c
C_SOURCES += $HUBDIR/hub-server.c
C_SOURCES += HUBDIR/hub-server-sock.c
C_SOURCES += "$SANDBOX.c" 
C_SOURCES += $HUBDIR/hub-server-queue.c 
C_SOURCES += $HUBDIR/hub-server-util.c 

-pthread -I$HUBDIR -I$CANDIR  -I ~/svn_discoveryf4/sw_discoveryf4/trunk/lib -L$CANDIR/PC -lcommon_all
 
#######################################
# CFLAGS
#######################################

PATHCANSERVER =../../../../can-server
C_INCLUDES = -I.
C_INCLUDES += -I$(PATHCANSERVER)/
-I$HUBDIR
 -I$CANDIR
   -I ~/svn_discoveryf4/sw_discoveryf4/trunk/lib
   
gcc -Wall -I.  -o "hub-server-bridge-$SANDBOX" $HUBDIR/hub-server.c $HUBDIR/hub-server-sock.c "$SANDBOX.c" $HUBDIR/hub-server-queue.c $HUBDIR/hub-server-util.c -pthread -I$HUBDIR -I$CANDIR  -I ~/svn_discoveryf4/sw_discoveryf4/trunk/lib -L$CANDIR/PC -lcommon_all




# compile gcc flags
ASFLAGS = $(AS_DEFS) $(AS_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections 

CFLAGS = $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections -Wno-parentheses

# libraries
LIBS = -lc -lm

CC = gcc
LDFLAGS = 

all: hub-server-bridge-$(SANDBOX)

hub-server-bridge-$(SANDBOX): $(sourcefiles)
	$(CC) $(CFLAGS) $(DEFS) $(CPPFLAGS) $(LDFLAGS) -I . -I ./include -o $(executable) $(sourcefiles) $(LIBS)


# *** EOF ***

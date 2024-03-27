.PHONY: all, clean

# Disable implicit rules
.SUFFIXES:

# Keep intermediate files
#.PRECIOUS: %.o

CC = gcc
CFLAGS = -Wall -g
LDFLAGS =

# Note: -lnsl does not seem to work on Mac OS but will
# probably be necessary on Solaris for linking network-related functions 
#LIBS += -lsocket -lnsl -lrt
LIBS += -lpthread

INCLUDE = csapp.h
OBJS = csapp.o echo.o
INCLDIR = -I.

PROGS = server client


all: clean $(PROGS) clean_o

%.o: %.c $(INCLUDE)
	$(CC) $(CFLAGS) $(INCLDIR) -c -o $@ $<
	
client: client.o $(OBJS)
	$(CC) -o for_client_exchange/$@ $(LDFLAGS) $^ $(LIBS)

%: %.o $(OBJS)
	$(CC) -o $@ $(LDFLAGS) $^ $(LIBS)

clean_o:
	rm -f *.o

clean_storage:
	rm -f for_client_exchange/*
	
clean:
	rm -f $(PROGS) *.o for_client_exchange/client

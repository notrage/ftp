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
OBJS = csapp.o
INCLDIR = -I.

PROGS = server client


all: clean $(PROGS) clean_o

%.o: %.c $(INCLUDE)
	$(CC) $(CFLAGS) $(INCLDIR) -c -o $@ $<
	
client: client.o $(OBJS)
	mkdir -p client_app
	mkdir -p client_app/files
	$(CC) -o client_app/$@ $(LDFLAGS) $^ $(LIBS)

server: server.o $(OBJS)
	mkdir -p server_app
	mkdir -p server_app/files
	$(CC) -o server_app/$@ $(LDFLAGS) $^ $(LIBS)

%: %.o $(OBJS)
	$(CC) -o $@ $(LDFLAGS) $^ $(LIBS)

clean_o:
	rm -f *.o

clean_storage: 
	rm -rf client_app/files/*
	
clean:
	rm -f $(PROGS) *.o client_app/client
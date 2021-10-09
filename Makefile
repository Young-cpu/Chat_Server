# See Makefile-commented for explanation
CFLAGS = -Wall -g
CC     = gcc $(CFLAGS)

PROGRAMS = \
	bl_server \
	bl_client

all: $(PROGRAMS)

server_funcs.o: server_funcs.c blather.h
	$(CC) -c $<

bl_server.o: bl_server.c blather.h
	$(CC) -c $<

bl_client.o: bl_client.c blather.h
	$(CC) -c $<
simpio.o: simpio.c blather.h
	$(CC) -c $<

#compiling privoided file util
util.o: util.c blather.h
	$(CC) -c $<

bl_client : bl_client.o server_funcs.o simpio.o util.o -lpthread
	$(CC) -o $@ $^

bl_server : bl_server.o server_funcs.o simpio.o util.o -lpthread
	$(CC) -o $@ $^

#clean up the all the existing files
clean :
	rm -f *.o *.fifo $(PROGRAMS)

#Test case
include test_Makefile
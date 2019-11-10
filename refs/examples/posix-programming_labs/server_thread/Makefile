INCD = inc ../common_macros/inc
INCD += ../struct_buffer/inc ../struct_bsd/inc
INCD += ../db_interface/inc

LIBS = ../server_lib/libserver.a
LIBS += ../fd_utils/libfdutils.a
LIBS += ../db_lock/libdblock.a ../db_simple/libdbsimple.a
LIBS += ../telnet_utils/libtelnet.a

TARGETEXEC = threadserver

all: $(TARGETEXEC)

SRC = src/thread_server.c src/client_thread.c

TESTINCD = $(INCD) ../telnet_utils/inc

include ../common_build/std.mk

.PHONY: run
run: server
	./server /tmp/db.socket

tests: tests/run
	tests/run

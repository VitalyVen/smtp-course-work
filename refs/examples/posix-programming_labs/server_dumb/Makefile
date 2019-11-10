# Includes directories:
INCD = inc ../common_macros/inc
INCD += ../struct_buffer/inc ../struct_bsd/inc
INCD += ../db_interface/inc

LIBS = ../db_simple/libdbsimple.a
LIBS += ../server_lib/libserver.a
LIBS += ../fd_utils/libfdutils.a
LIBS += ../telnet_utils/libtelnet.a

TARGETEXEC = dumbserver

all: $(TARGETEXEC)

SRC = src/dumb_server.c

TESTINCD = $(INCD) ../telnet_utils/inc

include ../common_build/std.mk

.PHONY: run
run: server
	./server /tmp/db.socket

tests: tests/run
	tests/run

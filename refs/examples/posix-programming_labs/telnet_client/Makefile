INCD = ../struct_buffer/inc ../common_macros/inc ../telnet_utils/inc

TARGETEXEC=client

all: $(TARGETEXEC)

SRC =

LIBS = ../fd_utils/libfdutils.a
LIBS += ../telnet_utils/libtelnet.a

.PHONY: run
run: client
	./client /tmp/db.socket

include ../common_build/std.mk

tests: tests/run
	tests/run

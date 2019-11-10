INCD = inc ../common_macros/inc
INCD += ../struct_bsd/inc ../struct_buffer/inc ../struct_wrbuffer/inc
INCD += ../db_interface/inc ../db_dlibs/inc ../telnet_utils/inc

LIBS = ../server_lib/libserver.a
LIBS += ../fd_utils/libfdutils.a
LIBS += ../telnet_utils/libtelnet.a

TARGETEXEC = pollserver

all: $(TARGETEXEC)

SRC = src/poll_server.c

include ../common_build/std.mk

include ../db_dlibs/libs.mk

.PHONY: run_simple run_parsem

run_simple: $(DBSIMPLE) server
	./server $< /tmp/db.socket 3

run_parsem: $(DBPARSEM) server
	./server $< /tmp/db.socket 3

run_parcnd: $(DBPARCND) server
	./server $< /tmp/db.socket 3

.PHONY: tests test_simple test_lock test_parsem test_parcnd

tests: test_simple test_lock test_parsem test_parcnd
	true

test_simple: $(DBSIMPLE) tests/run
	tests/run $<

test_parsem: $(DBPARSEM) tests/run
	tests/run $<

test_parcnd: $(DBPARCND) tests/run
	tests/run $<

test_lock: $(DBLOCK) tests/run
	tests/run $<

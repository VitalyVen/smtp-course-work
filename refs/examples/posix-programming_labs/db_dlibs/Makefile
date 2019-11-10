# Includes directories:
INCD = inc ../common_macros/inc
INCD += ../struct_bsd/inc ../struct_queue_sem/inc
INCD += ../db_interface/inc ../req_interface/inc
INCD += ../req_queue_sem/inc ../req_queue_cnd/inc
INCD += ../db_simple/inc ../db_lock/inc ../db_parallel/inc

# Default target is four shared libraries.
all: libdbparcnd.so libdbparsem.so libdblock.so libdbsimple.so

LIBS = ../db_parallel/libdbpar.a ../db_lock/libdblock.a ../db_simple/libdbsimple.a
LIBS += ../req_queue_sem/libreqsem.a ../req_queue_cnd/libreqcnd.a

include ../common_build/std.mk

.PHONY: test_simple test_lock test_parcnd test_parsem
tests: test_simple test_lock test_parcnd test_parsem

test_simple: tests/run ./libdbsimple.so
	tests/run ./libdbsimple.so

test_lock: tests/run ./libdblock.so
	tests/run ./libdblock.so

test_parcnd: tests/run ./libdbparcnd.so
	tests/run ./libdbparcnd.so

test_parsem: tests/run ./libdbparsem.so
	tests/run ./libdbparsem.so

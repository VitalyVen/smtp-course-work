# Includes directories:
INCD = inc ../common_macros/inc
INCD += ../struct_buffer/inc ../struct_bsd/inc ../struct_wrbuffer/inc
INCD += ../db_interface/inc
INCD += ../telnet_utils/inc

# Default target is a static library.
TARGETLIB = libserver.a
TARGETEXEC = sample
all: $(TARGETLIB) $(TARGETEXEC)

# Object files
SRC = src/cnct_io.c src/cnct.c src/cmds.o src/wrbuf_io.c src/sig_handler.c

LIBS = ../fd_utils/libfdutils.a ../db_simple/libdbsimple.a

TESTINCD = $(INCD) ../struct_bsd/inc ../req_interface/inc

TESTLIBS = $(LIBS)
TESTLIBS += ../db_simple/libdbsimple.a
TESTLIBS += ../db_parallel/libdbpar.a
TESTLIBS += ../req_queue_cnd/libreqcnd.a
TESTLIBS += ../telnet_utils/libtelnet.a

include ../common_build/std.mk

# Targets for testing.
.PHONY: tests_memory

tests_memory: tests/run
	valgrind tests/run

# Includes directories:
INCD = inc ../common_macros/inc ../struct_bsd/inc
INCD += ../db_interface/inc ../req_interface/inc
INCD += ../db_simple/inc

# Default target is a static library.
TARGETLIB = libdbpar.a
all: $(TARGETLIB)

# Source files
SRC = src/db_parallel.c src/string_hash.c

TESTLIBS +=../db_simple/libdbsimple.a ../req_queue_cnd/libreqcnd.a

include ../common_build/std.mk

tests: tests/run
	tests/run

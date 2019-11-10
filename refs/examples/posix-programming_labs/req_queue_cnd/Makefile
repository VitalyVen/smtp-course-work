# Includes directories:
INCD = inc ../common_macros/inc
INCD += ../db_interface/inc ../req_interface/inc
INCD += ../struct_bsd/inc

# Default target is a shared library.
TARGETLIB = libreqcnd.a

all: $(TARGETLIB)

# Source files
SRC = src/req_cnd.c src/req_queue.c src/req_queue_cnd.c

include ../common_build/std.mk

tests: tests/run
	tests/run

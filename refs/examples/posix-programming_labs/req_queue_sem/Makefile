# Includes directories:
INCD = inc ../common_macros/inc
INCD += ../db_interface/inc ../req_interface/inc
INCD += ../struct_queue_sem/inc

# Default target is a static library.
TARGETLIB = libreqsem.a
all: $(TARGETLIB)

# Source files
SRC = src/req_queue_sem.c

include ../common_build/std.mk


tests: tests/run
	tests/run

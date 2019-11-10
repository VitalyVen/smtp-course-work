# Includes directories:
INCD = inc ../common_macros/inc
INCD += ../struct_buffer/inc
INCD += ../fd_utils/inc

# Default target is a static library.
TARGETLIB = libtelnet.a
all: $(TARGETLIB)

# Source files
SRC = src/telnet_io.c

LIBS = ../fd_utils/libfdutils.a

include ../common_build/std.mk


tests: tests/run
	tests/run

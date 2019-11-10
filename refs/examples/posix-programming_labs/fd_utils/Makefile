# Includes directories:
INCD = inc ../common_macros/inc

# Default target is a static library.
TARGETLIB = libfdutils.a
all: $(TARGETLIB)

# Object files
SRC = src/fd_utils.c

include ../common_build/std.mk

tests: tests/run
	tests/run

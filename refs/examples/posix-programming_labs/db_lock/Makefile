# Includes directories:
INCD = inc ../common_macros/inc ../struct_bsd/inc
INCD += ../db_simple/inc ../db_interface/inc

# Default target is a static library.
TARGETLIB = libdblock.a
all: $(TARGETLIB)

SRC = src/db_lock.c src/string_hash.c

LIBS = ../db_simple/libdbsimple.a

include ../common_build/std.mk

tests: tests/run
	tests/run

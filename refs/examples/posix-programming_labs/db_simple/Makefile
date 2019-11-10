# Includes directories:
INCD = inc ../db_interface/inc
INCD += ../common_macros/inc  ../struct_bsd/inc

# Default target is a static library.
TARGETLIB = libdbsimple.a

all: $(TARGETLIB)

SRC = src/db_simple.c

.PHONY: tests_memory

tests_memory: tests/run
	valgrind tests/run

include ../common_build/std.mk

tests: tests/run
	tests/run

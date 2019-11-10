INCD = inc ../struct_bsd/inc ../common_macros/inc

all:

include ../common_build/std.mk

.PHONY: test_memory

test_memory: tests/run
	valgrind tests/run

tests: tests/run
	tests/run

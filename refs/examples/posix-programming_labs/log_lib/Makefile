INCD = inc ../common_macros/inc ../struct_buffer/inc

TARGETLIB = liblogging.a
all: $(TARGETLIB)

SRC = src/process.c src/redir.c src/logger.c src/sig_log.c

LIBS=../fd_utils/libfdutils.a

.PHONY: tests_memory

tests_memory: tests/run
	valgrind --leak-check=full --show-reachable=yes tests/run

include ../common_build/std.mk

tests: tests/run
	tests/run

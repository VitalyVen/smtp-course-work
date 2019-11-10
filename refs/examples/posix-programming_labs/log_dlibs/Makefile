INCD = inc ../common_macros/inc
LIBS = ../log_lib/liblogging.a ../fd_utils/libfdutils.a

all: liblogdumb.so liblogsyslog.so liblogfile.so liblogfile2.so

include ../common_build/std.mk

.PHONY: tests_memory

tests: tests/run all
	tests/run ./liblogdumb.so
	tests/run ./liblogfile.so
	tests/run ./liblogfile2.so
	tests/run ./liblogsyslog.so


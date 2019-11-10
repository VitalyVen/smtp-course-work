INCD = inc ../common_macros/inc

TARGETLIB = liblogsetup.a
TARGETEXEC = sample

all: $(TARGETEXEC) $(TARGETLIB)

SRC = src/log_setup.c

.PHONY: run run_dumb run_syslog run_file run_file2

run: run_dumb run_syslog run_file run_file2

run_dumb: sample ../log_dlibs/liblogdumb.so
	./sample ../log_dlibs/liblogdumb.so

run_syslog: sample ../log_dlibs/liblogsyslog.so
	./sample ../log_dlibs/liblogsyslog.so:notice,alert

run_file: sample ../log_dlibs/liblogfile.so
	$(RM) sample.log
	./sample ../log_dlibs/liblogfile.so:sample.log
	cat sample.log

run_file2: sample ../log_dlibs/liblogfile2.so
	$(RM) sample.log
	./sample ../log_dlibs/liblogfile2.so:sample.log
	cat sample.log

fail: sample
	$(RM) sample.log
	./sample ../log_dlibs/invalid.so:sample.log

include ../common_build/std.mk

tests: tests/run ../log_dlibs/liblogfile.so ../log_dlibs/liblogfile2.so
	tests/run ../log_dlibs/liblogfile.so:/dev/null
	tests/run ../log_dlibs/liblogfile2.so:/dev/null

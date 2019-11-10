INCD = inc ../common_macros/inc

TARGETLIB = libservice.a
TARGETEXEC = sample

all: $(TARGETEXEC)

SRC = src/service.c src/pidfile.c

LIBS=../fd_utils/libfdutils.a

.PHONY: run_sample
PIDFILE = /tmp/sample_runner.pid
LOG = $(shell pwd)/sample.log

start_sample: $(TARGETEXEC)
	$(RM) $(LOG)
	./$(TARGETEXEC) start $(PIDFILE) \
	    sh -c 'while true; do d="$$(date)"; echo $$d >> $(LOG); sleep 1; done'

stop_sample: $(TARGETEXEC)
	./$(TARGETEXEC) stop $(PIDFILE)

sample_status: $(TARGETEXEC)
	./$(TARGETEXEC) status $(PIDFILE)

include ../common_build/std.mk

tests: tests/run
	tests/run

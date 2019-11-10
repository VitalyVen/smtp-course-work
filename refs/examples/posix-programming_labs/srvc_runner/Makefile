INCD = inc ../common_macros/inc

TARGETEXEC = runner

all: $(TARGETEXEC)

SRC =

LIBS = ../log_setup/liblogsetup.a ../srvc_lib/libservice.a
LIBS += ../fd_utils/libfdutils.a

.PHONY: run_sample
PIDFILE = $(shell pwd)/sample.pid
LOGLIB = $(shell pwd)/../log_dlibs/liblogfile2.so
LOGFILE = $(shell pwd)/sample.log

rm_log:
	$(RM) $(LOGFILE)

view_log:
	cat $(LOGFILE)

start_sample: $(TARGETEXEC) $(LOGLIB)
	./$(TARGETEXEC) start $(PIDFILE) $(LOGLIB):$(LOGFILE) \
	    /bin/sh -c 'while true; do d="$$(date)"; echo $$d; sleep 1; done'

stop_sample: $(TARGETEXEC)
	./$(TARGETEXEC) stop $(PIDFILE)

sample_status: $(TARGETEXEC)
	./$(TARGETEXEC) status $(PIDFILE)

include ../common_build/std.mk

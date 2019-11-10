RUNNER = ../srvc_runner/runner

all:

PIDFILE = /tmp/service_db.pid
SOCKET = /tmp/db.socket
MAXCNT = 3
LOG = /tmp/sample_db_server.log
LIBFILELOG = ../log_dlibs/liblogfile.so
LIBFILELOG2 = ../log_dlibs/liblogfile2.so
LIBSYSLOG = ../log_dlibs/liblogsyslog.so
POLLSERVER = $(shell pwd)/../server_poll/pollserver
SIMPLEDBLIB = $(shell pwd)/../db_dlibs/libdbsimple.so
PARSEMDBLIB = $(shell pwd)/../db_dlibs/libdbparsem.so
PARCNDDBLIB = $(shell pwd)/../db_dlibs/libdbparcnd.so
THREADSERVER = $(shell pwd)/../server_thread/threadserver

view_log:
	cat $(LOG)

start_pollserver_syslog: $(RUNNER) $(POLLSERVER) $(LIBSYSLOG) $(SIMPLEDBLIB)
	$(RM) $(LOG)
	./$(RUNNER) start $(PIDFILE) $(LIBSYSLOG):$(LOG) $(POLLSERVER) $(SIMPLEDBLIB) $(SOCKET) $(MAXCNT)

start_pollserver_filelog: $(RUNNER) $(POLLSERVER) $(LIBFILELOG) $(PARSEMDBLIB)
	$(RM) $(LOG)
	./$(RUNNER) start $(PIDFILE) $(LIBFILELOG):$(LOG) $(POLLSERVER) $(PARSEMDBLIB) $(SOCKET) $(MAXCNT)

start_pollserver_filelog2: $(RUNNER) $(POLLSERVER) $(LIBFILELOG2) $(PARCNDDBLIB)
	$(RM) $(LOG)
	./$(RUNNER) start $(PIDFILE) $(LIBFILELOG2):$(LOG) $(POLLSERVER) $(PARCNDDBLIB) $(SOCKET) $(MAXCNT)

start_threadserver_syslog: $(RUNNER) $(THREADSERVER) $(LIBSYSLOG)
	$(RM) $(LOG)
	./$(RUNNER) start $(PIDFILE) $(LIBSYSLOG):notice,alert $(THREADSERVER) $(SOCKET)

start_threadserver_filelog: $(RUNNER) $(THREADSERVER) $(LIBFILELOG)
	$(RM) $(LOG)
	./$(RUNNER) start $(PIDFILE) $(LIBFILELOG):$(LOG) $(THREADSERVER) $(SOCKET)

start_threadserver_filelog2: $(RUNNER) $(THREADSERVER) $(LIBFILELOG2)
	$(RM) $(LOG)
	./$(RUNNER) start $(PIDFILE) $(LIBFILELOG2):$(LOG) $(THREADSERVER) $(SOCKET)

stop: $(RUNNER)
	./$(RUNNER) stop $(PIDFILE)

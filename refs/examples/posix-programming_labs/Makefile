.PHONY: all tests clean

SUBDIRS = common_macros db_dlibs db_interface db_lock db_parallel db_simple fd_utils log_dlibs log_lib log_setup req_interface req_queue_cnd req_queue_sem server_lib server_dumb server_poll server_thread srvc_lib srvc_runner struct_buffer struct_queue_sem struct_wrbuffer telnet_client telnet_utils

.PHONY: $(SUBDIRS)

all: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) $(MFLAGS) -C $@

telnet_utils: fd_utils

db_parallel: db_simple req_queue_cnd req_queue_sem
db_lock: db_simple
db_dlibs: db_simple db_lock db_parallel

server_lib: db_simple telnet_utils
server_dumb: server_lib telnet_utils db_simple
server_thread: server_lib telnet_utils db_lock
server_poll: server_lib telnet_utils db_dlibs


log_lib: fd_utils

log_dlibs: log_lib

srvc_lib: fd_utils

srvc_runner: srvc_lib log_dlibs log_setup
srvc_db: srvc_runner server_poll server_thread db_dlibs

telnet_client: telnet_utils

clean:
	for dir in $(SUBDIRS); do \
	    $(MAKE) $(MFLAGS) -C $$dir clean; \
	    RETVAL=$$?; \
	    [ $$RETVAL -eq 0 ] || exit 1; \
	done

tests: all
	for dir in $(SUBDIRS); do \
	    $(MAKE) $(MFLAGS) -C $$dir tests; \
	    RETVAL=$$?; \
	    [ $$RETVAL -eq 0 ] || exit 1; \
	done

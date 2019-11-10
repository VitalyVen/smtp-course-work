#ifndef __SERVICE_H__
#define __SERVICE_H__

#include <sys/types.h>

pid_t daemonize(void);

pid_t fork_service(char *argv[]);

int wait_service(pid_t pid);

int stop_service(const char *pidfile);

void set_sighandler_pid(pid_t pid);

int set_sighandler();

#endif

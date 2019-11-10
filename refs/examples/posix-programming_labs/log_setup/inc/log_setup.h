#ifndef __LOG_SETUP_H__
#define __LOG_SETUP_H__

#include <sys/types.h>

// On error, NULL is returned.
// On success, result of dlopen() is return;
void *setup_log(char *progname, char *log);

void close_log(void *log_lib);

extern pid_t (*out_pid)(void);
extern pid_t (*err_pid)(void);

#endif

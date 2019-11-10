#ifndef __LOGLIB_H__
#define __LOGLIB_H__

#include <sys/types.h>

// Returns plugin name.
const char *name();

// On success, returns 1 in parent process and 0 in child process.
// On error, -1 has returned.
int start(char *progname, char *param);

void stop(void);

pid_t out_pid(void);
pid_t err_pid(void);

#endif

#ifndef __FORK_LOG_H__
#define __FORK_LOG_H__

#include <sys/types.h>

typedef void (*process_t)(char *message);

ssize_t process_buffer(char buf[], process_t process);

void process_input(int fd, process_t process, ssize_t buf_size);

int poll_inputs(int n, int fd[], process_t process[], ssize_t buf_size);

#endif

#ifndef __REDIR_H__
#define __REDIR_H__

#include <stdio.h>
#include <sys/types.h>

#include "process.h"

struct redir {
    int saved_fd, fd;
    int pipe_in, pipe_out;
    FILE *f;
    pid_t pid;
    process_t process;
};

int init_redir(struct redir *r, process_t process, FILE *f);

int create_pipe(struct redir *r);

int activate_pipe(struct redir *r);

// int close_child_pipe(struct redir *r)

int restore_redir(struct redir *r);

void restore_broken_redir(struct redir *r);

void waitpid_redir(struct redir *r);

// On success, child pid is returned (0 is returned in child process).
// On error, -1 is returned.
pid_t fork_redir(struct redir *r);

pid_t fork_redirs(struct redir r[], int n);

#endif

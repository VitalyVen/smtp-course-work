#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "errors.h"
#include "fd_utils.h"
#include "redir.h"

#define BUFSIZE 4096

int init_redir(struct redir *r, process_t process, FILE *f)
{
    r->pid = -1;
    r->fd = fileno(f);
    if (r->fd == -1) {
        errnomsg("fileno()");
        return -1;
    }
    if ((r->saved_fd = dup(r->fd)) == -1) {
        errnomsg("dup(%d)", r->fd);
        return -1;
    }
    r->f = f;
    r->process = process;
    return 0;
}

int create_pipe(struct redir *r)
{
    int pipefds[2];
    if (pipe(pipefds)) {
        errnomsg("pipe()");
        return -1;
    }
    debug("pipe: %d -> %d", pipefds[1], pipefds[0]);
    r->pipe_out = pipefds[0];
    r->pipe_in = pipefds[1];
    return 0;
}

int activate_pipe(struct redir *r)
{
    fflush(r->f);
    while (dup2(r->pipe_in, r->fd) == -1)
        CHECK_EINTR(, return -1, "dup2(%d, %d)", r->pipe_in, r->fd);
// TODO: Q. It does not help after fork/exec.
//     if (r->fd == STDOUT_FILENO)
//         setlinebuf(stdout);
//     if (r->fd == STDERR_FILENO)
//         setlinebuf(stderr);
    return safe_close(r->pipe_out);
}

int restore_redir(struct redir *r)
{
    fflush(r->f);
    if (r->saved_fd != -1) {
        // TODO: Q: what happens without the following line?
        safe_close(r->pipe_in);
        debug("dup2: %d => %d", r->saved_fd, r->fd);
        // Note: closing r->fd is unnessecary accroding to "man dup2" ?
        while (dup2(r->saved_fd, r->fd) == -1)
            CHECK_EINTR(, return -1, "dup2(%d, %d)", r->saved_fd, r->fd);
        r->saved_fd = -1;
        return 0;
    }
    return -1;
}

void restore_broken_redir(struct redir *r)
{
    if (r->saved_fd != -1)
        if (kill(r->pid, 0)) { // Check if process does not exist.
            restore_redir(r);
            r->pid = -1;
        }
}

void waitpid_redir(struct redir *r)
{
    if (r->pid != -1) {
        if (waitpid(r->pid, NULL, 0) == -1)
        if (!(errno == ECHILD || errno == EINTR))
            errnomsg("waitpid()");
        r->pid = -1;
    }
}

pid_t fork_redir(struct redir *r)
{
    if (create_pipe(r))
        return -1;
    r->pid = fork();
    if (r->pid < 0) {
        errnomsg("fork()");
        return -1;
    }
    if (r->pid > 0) { // parent
        activate_pipe(r);
        return r->pid;
    }
    // child
    // FIXME: error checking, no?
    safe_close(r->pipe_in);
    debug("processing...");
    process_input(r->pipe_out, r->process, BUFSIZE);
    debug("closing...");
    safe_close(r->pipe_out);
    return 0;
}

int fork_redirs(struct redir r[], int n)
{
    pid_t pid;
    for (int i = 0; i < n ; i++)
        if (create_pipe(r + i))
            return -1;
    if ((pid = fork()) < 0) {
        perror("fork()");
        return -1;
    }
    if (pid > 0) { // parent
        for (int i = 0; i < n ; i++) {
            activate_pipe(r + i);
            r[i].pid = pid;
        }
        return pid;
    }
    else { // child
//         for (int i = 0; i < n ; i++)
//             restore_stdfds(r + i);
        int fd[n];
        process_t process[n];
        for (int i = 0; i < n ; i++) {
            safe_close(r[i].pipe_in);
            fd[i] = r[i].pipe_out;
            process[i] = r[i].process;
        }
        poll_inputs(n, fd, process, BUFSIZE);
        for (int i = 0; i < n ; i++)
            safe_close(r[i].pipe_out);
        return 0;
    }
}

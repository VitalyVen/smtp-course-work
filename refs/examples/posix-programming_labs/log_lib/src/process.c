#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include "fd_utils.h"
#include "process.h"
#include "errors.h"

#define OVERRUN "..."

ssize_t process_buffer(char buf[], process_t process)
{
    char *ln, *cur = buf;
    while ((ln = strchr(cur, '\n'))) {
        char *msg = strndup(cur, ln - cur + 1);
        if (!msg)
            errnomsg("strndup()");
        else {
            process(msg);
            free(msg);
        }
        cur = ln + 1;
    }
    ssize_t n = strlen(cur);
    // TODO: Q.: why not a strcpy?
    // A. Overlapping.
    memmove(buf, cur, n + 1);
    return n;
}

void process_input(int fd, process_t process, ssize_t buf_size)
{
    char buf[buf_size];
    ssize_t pos = 0;
    while (1) {
        if (buf_size - pos - 1 == 0) {
            warning("buffer overrun [size = %ld]", (long) buf_size);
            pos = strlen(strcpy(buf, OVERRUN));
        }
        debug("reading input [fd = %d]...", fd);
        ssize_t len = read(fd, buf + pos, buf_size - pos - 1);
        if (len == 0)
            return;
        if (len == -1)
            CHECK_EINTR(, return, "read(%d)", fd)
        else {
            len += pos;
            buf[len] = 0;
            pos = process_buffer(buf, process);
        }
    }
}

int process_nonblock(int fd, process_t process, char *buf, ssize_t buf_size)
{
    int closing = 0;
    while (!closing) {
        ssize_t pos = strlen(buf);
        if (buf_size - pos - 1 == 0) {
            warning("buffer overrun [size = %d]", (int) buf_size);
            pos = strlen(strcpy(buf, OVERRUN));
        }
        debug("reading input [fd = %d]...", fd);
        ssize_t len = read(fd, buf + pos,
                           buf_size - pos - 1);
        if (len == -1)
            CHECK_EINTREAGAIN(continue, break, closing = 1,
                              "read(%d)", fd);
        if (len == 0) {
            debug("connection closed: %d", fd);
            closing = 1;
        }
        if (len > 0) {
            len += pos;
            buf[len] = '\0';
            process_buffer(buf, process);
        }
    }
    return closing;
}

int poll_inputs(int n, int fd[], process_t process[], ssize_t buf_size)
{
    struct pollfd poll_fds[n];
    char buf[n][buf_size];
    for (int i = 0; i < n; i++) {
        poll_fds[i].fd = fd[i];
        if (set_nonblock(fd[i])) {
            errnomsg("setnonblock(%d)", fd[i]);
            return -1;
        }
        poll_fds[i].events = POLLIN | POLLRDNORM;
        buf[i][0] = '\0';
    }
    int nfds = n;
    while (nfds > 0) {
        int nevents = poll(poll_fds, n, -1);
        if (nevents == -1)
            CHECK_EINTR(continue, return -1, "poll()");
        assert(nfds >= nevents);
        debug("events: %d, n: %d", nevents, n);
        for (int i = 0; nevents > 0 && i < n; i++) {
            if (poll_fds[i].fd == -1 || !poll_fds[i].revents)
                continue;
            nevents--;
            int closing = 0;
            if (poll_fds[i].revents & (POLLIN | POLLRDNORM))
                closing = process_nonblock(poll_fds[i].fd, process[i],
                                           buf[i], buf_size);
            if (poll_fds[i].revents & (POLLHUP | POLLERR)) {
                closing = 1;
            }
            if (closing) {
                debug("closing fd: %d", poll_fds[i].fd);
                // FIXME: closing is useles?
//                 safe_close(poll_fds[i].fd);
                poll_fds[i].fd = -1;
                nfds--;
            }
            if (poll_fds[i].revents & ~(POLLIN | POLLRDNORM | POLLHUP | POLLERR))
                error("Unexpected events: %d", poll_fds[i].revents);
        }
    }
    return 0;
}

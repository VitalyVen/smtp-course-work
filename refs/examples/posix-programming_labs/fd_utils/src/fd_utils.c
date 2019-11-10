#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "fd_utils.h"
#include "errors.h"

int create_binded_socket(char *socket_name)
{
    int sock = socket(PF_LOCAL, SOCK_STREAM, 0);
    if (sock == -1) {
        errnomsg("socket()");
        return -1;
    }
    struct sockaddr_un address = {.sun_family = AF_LOCAL};
    if (sizeof(address.sun_path) <= strlen(socket_name)) {
        error("socket name is too long (limit = %lu)\n",
              (long unsigned) sizeof(address.sun_path) - 1);
        return -1;
    }
    strcpy(address.sun_path, socket_name);
    if (unlink(socket_name) && errno != ENOENT) { // ENOENT is fine.
        errnomsg("unlink()");
        return -1;
    }
    if (bind(sock, (struct sockaddr *) &address, sizeof(address))) {
        errnomsg("bind()");
        return -1;
    }
    if (listen(sock, SOMAXCONN)) {
        errnomsg("listen()");
        return -1;
    }
    return sock;
}

ssize_t send_all(int sockfd, const void *buf, size_t len)
{
    size_t sent = 0;
    while (sent < len) {
        if (sent > 0)
            debug("partial sent (%zd of %zd)", sent, len);
        debug("send(%d)", (int) (len - sent));
        ssize_t dsent = send(sockfd, buf + sent, len - sent, MSG_NOSIGNAL);
        debug("dsent = %d", (int) dsent);
        if (dsent == -1)
            CHECK_EINTR(dsent = 0, return -1, "send(%d)", sockfd);
        sent += dsent;
    };
    return sent;
}

ssize_t safe_recv(int sockfd, char *buf, size_t len)
{
    ssize_t rcvd;
    rcvd = recv(sockfd, buf, len, 0);
    if (rcvd == -1)
        CHECK_EINTREAGAIN(return 0, return 0, return -1,
                          "recv(%d)", sockfd);
    if (rcvd == 0) // Closed connection.
        return -1;
    return rcvd;
}

ssize_t safe_send(int sockfd, const void *buf, size_t len)
{
    ssize_t nsent;
    nsent = send(sockfd, buf, len, MSG_NOSIGNAL);
    if (nsent == -1)
        CHECK_EINTREAGAIN(return 0, return 0, return -1,
                          "send(%d)", sockfd);
    return nsent;
}

int safe_close(int fd)
{
    while (close(fd) == -1)
        CHECK_EINTR(, return -1, "close(%d)", fd);
    return 0;
}

int set_nonblock(int fd)
{
    int cntl = fcntl(fd, F_GETFL, 0);
    if (cntl == -1) {
        errnomsg("fcntl(F_GETFL)");
        return -1;
    }
    if (fcntl(fd, F_SETFL, cntl | O_NONBLOCK) == -1) {
        errnomsg("fcntl(F_SETFL)");
        return -1;
    }
    return 0;
}

ssize_t read_all(int fd, void *buf, size_t len)
{
    size_t n = 0;
    while (n < len) {
        ssize_t dn = read(fd, buf + n, len - n);
        if (dn == -1)
            CHECK_EINTREAGAIN(, if (!n) return 0; else usleep(1),
                              return -1, "read(%d)", fd)
        else
            n += dn;
    }
    return n;
}

ssize_t write_all(int fd, const void *buf, size_t len)
{
    size_t n = 0;
    while (n < len) {
        ssize_t dn = write(fd, buf + n, len - n);
        if (dn == -1)
            CHECK_EINTR(, return -1, "write(%d)", fd)
        else
            n += dn;
    };
    return n;
}

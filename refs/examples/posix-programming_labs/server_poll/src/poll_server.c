#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "cmds.h"
#include "errors.h"
#include "fd_utils.h"
#include "cnct_io.h"
#include "poll_server.h"
#include "sig_handler.h"
#include "telnet_msg.h"
#include "wrbuf_io.h"

#define TOTALFD(server) ((server)->max_cnct + 2)

void send_answer(struct poll_cnct *cnct, char *value)
{
    struct answer request;
    request.nfd = cnct->cnct.pollfd - cnct->server->fds;
    request.value = value; // Valgrind hates this.
    ssize_t wrlen = write_all(cnct->server->control_input, &request, sizeof(request));
    assert(wrlen == sizeof(request));
}

void async_response(char *key, char *value, void *arg)
{
    struct poll_cnct *cnct = arg;
    assert(cnct);
    assert(cnct->cnct.pollfd);
    // Uneffective: send_telnet(cnct_fd(cnct), value); dec_queue_cnt(..).
    // Checking (cnct_fd(&cnct->cnct) != -1) causes barrier error
    // so we'll always send write request.
    debug("Write request sent: \"%s\"", value);
    // TODO: Q. is deadlock possible?
    // A. Yes, with single_threaded db.
    send_answer(cnct, value);
    free(key);
}

int init_server(struct poll_server *server, int max_cnct,
            struct vdb *db)
{
    int ncncts = 0;
    memset(server, sizeof(*server), 0);
    int link[2];
    if (pipe(link)) {
        errnomsg("pipe()");
        goto err_pipe;
    }
    server->control_input = link[1];
    server->max_cnct = max_cnct;
    server->db = db;
    server->wrbufs = calloc(server->max_cnct,
                            sizeof(server->wrbufs[0]));
    if (!server->wrbufs) {
        errnomsg("calloc()");
        goto err_wrbufs;
    }
    for (int i = 0; i < server->max_cnct; i++)
        init_wrbuffer(server->wrbufs + i);
    server->fds = calloc(TOTALFD(server),
                         sizeof(server->fds[0]));
    if (!server->fds) {
        errnomsg("calloc()");
        goto err_fds;
    }
    server->controlfd = server->fds + server->max_cnct + 1;
    server->bindedfd = server->fds + server->max_cnct;
    for (int i = 0; i < TOTALFD(server); i++) {
        server->fds[i].events = POLLIN;
        // TODO: how about | POLLRDNORM | POLLRDBAND | POLLPRI;
        server->fds[i].fd = -1;
    }
    server->cncts = calloc(server->max_cnct, sizeof(server->cncts[0]));
    if (!server->cncts) {
        errnomsg("calloc()");
        goto err_cncts;
    }
    server->controlfd->fd = link[0];
    set_nonblock(server->controlfd->fd);
    for (ncncts = 0; ncncts < server->max_cnct; ncncts++) {
        if (init_poll_cnct(CNCT(server, ncncts),
                           server->fds + ncncts) == -1)
            goto err_ncncts;
        server->cncts[ncncts].server = server;
    }
    return 0;
err_ncncts:
    for (int i = 0; i < ncncts; i++)
        destroy_cnct(CNCT(server, i));
    free(server->cncts);
err_cncts:
    free(server->fds);
err_fds:
    free(server->wrbufs);
err_wrbufs:
    safe_close(link[1]);
    safe_close(link[0]);
err_pipe:
    return -1;
}

void destroy_server(struct poll_server *server)
{
    // TODO: Assignment: server should not destroy DB
    // (but it still should sync correctly with
    // db worker threads, that's a problem).
    if (server->db)
        server->db->destroy(server->db);
    for (int i = 0; i < server->max_cnct; i++)
        destroy_cnct(CNCT(server, i));
    for (int i = 0; i < server->max_cnct; i++)
        destroy_wrbuffer(server->wrbufs + i);
    safe_close(server->controlfd->fd);
    safe_close(server->control_input);
    free(server->wrbufs);
    free(server->fds);
    free(server->cncts);
}

int add_fd(struct poll_server *server, int socket)
{
    for (int i = 0; i < server->max_cnct; i++) // Linear search. (
        if (cnct_state(CNCT(server, i)) == CNCT_SHUT) { // Free slot.
            server->fds[i].fd = socket;
            reset_poll_cnct(CNCT(server, i));
            return i;
        }
    return -1;
}

int accept_connection(struct poll_server *server)
{
    int new_socket;
    do {
        debug("accept() in blocking mode...");
        new_socket = accept(server->bindedfd->fd, NULL, 0);
        if (new_socket < 0)
            CHECK_EINTR(continue, return -1,
                        "accept(%d)", server->bindedfd->fd);
    } while (new_socket == -1);
    int nslot = add_fd(server, new_socket);
    if (nslot != -1) {
        debug("New connection %d.", new_socket);
        set_nonblock(new_socket);
    }
    else {
        warning("Clients limit reached");
        if (close(new_socket))
            errnomsg("close()");
    }
    return nslot;
}

void remove_connection(struct poll_server *server, int nfd)
{
    if (FD(server, nfd) == -1)
        return;
    safe_close(server->fds[nfd].fd);
    debug("Connection %d closed", FD(server, nfd));
    shutdown_cnct(CNCT(server, nfd));
    FD(server, nfd) = -1;
    reset_wrbuffer(server->wrbufs + nfd);
}

int bind_server(struct poll_server *server, char *socket_name)
{
    int binded_socket = create_binded_socket(socket_name);
    if (binded_socket < 0) {
        error("There is a failure when trying to use a socket \"%s\".",
                socket_name);
        return -1;
    }
    server->bindedfd->fd = binded_socket;
    return 0;
}

void process_control(struct poll_server *server)
{
    assert(server->controlfd->revents);
    if (server->controlfd->revents & POLLIN)
        while (1) {
            struct answer request;
            ssize_t nrecv = read_all(server->controlfd->fd, &request, sizeof(request));
            if (nrecv == -1)
                exit(0);
            if (nrecv == 0)
                break;
            dec_queue_cnt(CNCT(server, request.nfd));
            struct pollfd *reqfd = server->fds + request.nfd;
            if (reqfd->fd != -1) {
                // Just nice, we have some job to do (
                reqfd->events |= POLLOUT;
                debug("Events changed for fd %d: %04x", reqfd->fd, reqfd->events);
                if (request.value)
                    wrbuffer_add(server->wrbufs + request.nfd, request.value);
                wrbuffer_add(server->wrbufs + request.nfd, strdup(TELNET_SUFFIX));
            }
            else {
                debug("Write request is late.");
                free(request.value);
            }
        }
    if (server->controlfd->revents & ~POLLIN)
        warning("Unexpected event for control pipe.");
}

void process_client(struct poll_server *server, int nfd)
{
    struct pollfd *pollfd = server->fds + nfd;
    assert(pollfd->revents);
    int remove = 0;
    if (pollfd->revents & (POLLHUP | POLLERR))// Connection is closed by client.
        remove = 1;
    if ((!remove) && (pollfd->revents & POLLIN)) { // TODO: Q.
        debug("Input data, fd: %d", cnct_fd(CNCT(server, nfd)));
        ssize_t result = process_cnct(CNCT(server, nfd), server->db);
        if (result == -1) { //
            warning("Strange client detected (fd = %d).", pollfd->fd);
            remove = 1;
        }
    }
    if ((!remove) && (pollfd->revents & POLLOUT)) {
        ssize_t result = send_wrbuffer(server->wrbufs + nfd, pollfd->fd);
        if (result == -1) { //
            warning("Strange client detected (fd = %d).", pollfd->fd);
            remove = 1;
        }
        if (is_wrbuffer_empty(server->wrbufs + nfd)) {
            pollfd->events &= ~POLLOUT;
            debug("Events changed for fd %d: %04x", pollfd->fd, pollfd->events);
        }
    }
    if (remove)
        remove_connection(server, nfd);
    if (pollfd->revents & ~(POLLHUP | POLLERR | POLLIN | POLLOUT))
        warning("Unexpected event for socket %d.", pollfd->fd);
}

void process_fd(struct poll_server *server, int nfd)
{
    struct pollfd *pollfd = server->fds + nfd;
    if (pollfd == server->controlfd)
        process_control(server);
    else if (pollfd == server->bindedfd) { // A binded socket
        if (pollfd->revents | POLLIN) // Check a binded socket for a new client.
            accept_connection(server);
        if (pollfd->revents & ~POLLIN)
            warning("Unexpected event for binded socket.");
    }
    else
        process_client(server, nfd);
}

void poll_fds(struct poll_server *server)
{
    debug("Polling...");
    int nfds = poll(server->fds, TOTALFD(server), -1);
    debug("Number of polled slots: %d", nfds);
    if (nfds == -1)
        CHECK_EINTR(return, exit(0), "poll(%d)", TOTALFD(server));
    for (int i = TOTALFD(server) - 1; nfds > 0 && i >=0; i--) {
        struct pollfd *cur = server->fds + i;
        if (cur->fd == -1 || !cur->revents)
            continue;
        debug("Some events for slot: %d, fd: %d, events: %04x",
              i, cur->fd, cur->revents);
        nfds--;
        process_fd(server, i);
    }
}

void run_server(struct poll_server *server)
{
    if (set_termhandler())
        return;
    while (running)
        poll_fds(server);
    for (int i = 0; i < server->max_cnct; i++)
        remove_connection(server, i);
}

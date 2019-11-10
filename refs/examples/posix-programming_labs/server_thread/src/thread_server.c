#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "cmds.h"
#include "cnct.h"
#include "errors.h"
#include "fd_utils.h"
#include "cnct_io.h"
#include "sig_handler.h"
#include "thread_server.h"

int init_server(struct thread_server *server, struct vdb *db)
{
    int res;
    if ((res = pthread_rwlock_init(&server->state_lock, NULL))) {
        errmsg(res, "pthread_rwlock_init()");
        return -1;
    }
    if (init_threads(&server->threads) == -1) {
        if ((res = pthread_rwlock_destroy(&server->state_lock)))
            errmsg(res, "pthread_rwlock_destroy()");
        return -1;
    }
    server->db = db;
    server->binded_socket = -1;
    server->state = SERVER_RUNNING;
    return 0;
}

void destroy_server(struct thread_server *server)
{
    destroy_threads(&server->threads);
    int res;
    if ((res = pthread_rwlock_destroy(&server->state_lock)))
        errmsg(res, "pthread_rwlock_destroy()");
}

int bind_server(struct thread_server *server, char *socket_name)
{
    int binded_socket = create_binded_socket(socket_name);
    if (binded_socket < 0) {
        error("There is a failure when trying to use a socket \"%s\".",
              socket_name);
        return -1;
    }
    server->binded_socket = binded_socket;
    return 0;
}

static void *thread_func(struct client_thread *thread)
{
    struct cnct cnct;
    if (init_fd_cnct(&cnct, thread->socket))
        pthread_exit(NULL);
    while (running) {
        int result = process_cnct(&cnct, thread->server->db);
        if (result == -1)
            break;
    }
    safe_close(thread->socket);
    shutdown_cnct(&cnct);
    fatal(pthread_rwlock_rdlock(&thread->server->state_lock),
             "pthread_rwlock_rdlock()");
    if (thread->server->state == SERVER_RUNNING)
        remove_thread(&thread->server->threads, thread);
    fatal(pthread_rwlock_unlock(&thread->server->state_lock),
             "pthread_rwlock_unlock()");
    pthread_exit(NULL);
    return NULL; // non-reaches
}

void run_server(struct thread_server *server)
{
    if (set_termhandler())
        return;
    while (running) {
        debug("Accepting...\n");
        int new_socket = accept(server->binded_socket, NULL, 0);
        if (new_socket < 0)
            CHECK_EINTR(continue, return,
                        "accept(%d)", server->binded_socket);
        int res = new_thread(&server->threads, new_socket,
            server, thread_func);
        if (res)
            safe_close(new_socket);
    }
    fatal(pthread_rwlock_wrlock(&server->state_lock),
             "pthread_rwlock_wrlock()");
    server->state = SERVER_SHUTDOWN;
    fatal(pthread_rwlock_unlock(&server->state_lock),
            "pthread_rwlock_unlock()");
    join_threads(&server->threads);
}

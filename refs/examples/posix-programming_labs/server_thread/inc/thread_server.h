#ifndef __THREAD_SERVER_H__
#define __THREAD_SERVER_H__

#include <pthread.h>

#include "client_thread.h"
#include "db_virtual.h"
#include "queue.h"

enum server_state {
    SERVER_RUNNING,
    SERVER_SHUTDOWN
};

typedef enum server_state server_state_t;

// Multi-threaded server state structure.
struct thread_server {
    struct vdb *db;
    int binded_socket;     // Binded server socket.
    struct client_threads threads;
    server_state_t state;
    pthread_rwlock_t state_lock; // R/W lock to access state.
};

int init_server(struct thread_server *server, struct vdb *db);

void destroy_server(struct thread_server *server);

int bind_server(struct thread_server *server, char *socket_name);

void run_server(struct thread_server *server);

#endif

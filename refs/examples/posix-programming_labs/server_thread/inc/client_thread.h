#ifndef __CLIENT_THREAD_H__
#define __CLIENT_THREAD_H__

#include <pthread.h>

#include "queue.h"

struct thread_server;

struct client_thread {
    pthread_t id;
    struct thread_server *server;
    int socket;
    LIST_ENTRY(client_thread) link;
};

LIST_HEAD(threads_list, client_thread);

struct client_threads {
    int n;          // Number of threads.
    struct threads_list list;
    pthread_rwlock_t lock; // R/W lock to access threads list.
};

typedef void *(*thread_func_t)(struct client_thread *thread);

int init_threads(struct client_threads *threads);

void destroy_threads(struct client_threads *threads);

int new_thread(struct client_threads *threads, int socket,
               struct thread_server *server, thread_func_t func);

void remove_thread(struct client_threads *threads,
                   struct client_thread *thread);

void join_threads(struct client_threads *threads);

#endif

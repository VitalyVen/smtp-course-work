#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "client_thread.h"
#include "errors.h"

int init_threads(struct client_threads *threads)
{
    threads->n = 0;
    int res;
    if ((res = pthread_rwlock_init(&threads->lock, NULL))) {
        errmsg(res, "pthread_rwlock_init()");
        return -1;
    }
    LIST_INIT(&threads->list);
    return 0;
}

void destroy_threads(struct client_threads *threads)
{
    assert(threads->n == 0);
    int res;
    if ((res = pthread_rwlock_destroy(&threads->lock)))
        errmsg(res, "pthread_rwlock_destroy()");
}

int new_thread(struct client_threads *threads, int socket,
           struct thread_server *server, thread_func_t func)
{
    struct client_thread *thread = malloc(sizeof(*thread));
    if (!thread) {
        errnomsg("malloc()");
        return -1;
    }
    fatal(pthread_rwlock_wrlock(&threads->lock),
          "pthread_rwlock_wrlock()");
    threads->n++;
    debug("New connection: %d (no. clients: %d).", socket, threads->n);
    thread->socket = socket;
    thread->server = server;
    int res;
    if ((res = pthread_create(&thread->id, NULL,
                       (void *(*)(void *)) func, thread))) {
        errmsg(res, "pthread_create()");
        goto error;
    } else
        LIST_INSERT_HEAD(&threads->list, thread, link);
    fatal(pthread_rwlock_unlock(&threads->lock),
          "pthread_rwlock_unlock()");
    return 0;
error:
    free(thread);
    return -1;
}

static void remove_thread_unsafe(struct client_threads *threads, struct client_thread *thread)
{
    threads->n--;
    debug("Connection %d removed (no. clients: %d).",
          thread->socket, threads->n);
    LIST_REMOVE(thread, link);
    free(thread);
}

void remove_thread(struct client_threads *threads, struct client_thread *thread)
{
    fatal(pthread_rwlock_wrlock(&threads->lock),
          "pthread_rwlock_wrlock()");
    remove_thread_unsafe(threads, thread);
    fatal(pthread_rwlock_unlock(&threads->lock),
          "pthread_rwlock_unlock()");
}

void join_threads(struct client_threads *threads)
{
    struct client_thread *joined;
    fatal(pthread_rwlock_wrlock(&threads->lock),
          "pthread_rwlock_wrlock()");
    LIST_FOREACH(joined, &threads->list, link) {
        int res;
        if ((res = pthread_join(joined->id, NULL)))
            errmsg(res, "pthread_join()");
        remove_thread_unsafe(threads, joined);
    }
    assert(threads->n == 0);
    fatal(pthread_rwlock_unlock(&threads->lock),
          "pthread_rwlock_unlock()");
}

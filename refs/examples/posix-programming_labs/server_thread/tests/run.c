#include <assert.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>

#include <CUnit/Basic.h>

#include "buffer.h"
#include "db_lock.h"
#include "errors.h"
#include "fd_utils.h"
#include "misc_macros.h"
#include "telnet_io.h"
#include "telnet_msg.h"
#include "thread_server.h"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

#define MUTEX(statement) do { \
    assert(!pthread_mutex_lock(&mutex)); \
    statement; \
    assert(!pthread_mutex_unlock(&mutex));\
} while (0)

#define NTHREADS 100

int th_cnt;

void *simple_thread_func(struct client_thread *_)
{
    MUTEX(th_cnt++);
    pthread_exit(NULL);
    return NULL; // non-reaches
}

void test_simple_threads()
{
    struct client_threads threads;
    init_threads(&threads);
    th_cnt = 0;
    for (int i = 0; i < NTHREADS; i++)
        new_thread(&threads, -1, NULL, simple_thread_func);
    join_threads(&threads);
    MUTEX(CU_ASSERT_EQUAL(th_cnt, NTHREADS));
    CU_ASSERT_EQUAL(threads.n, 0);
    destroy_threads(&threads);
}

void *some_thread_func(struct client_thread *thread)
{
    assert(thread);
    assert(thread->server);
    if (thread->socket == -1)
        remove_thread(&thread->server->threads, thread);
    pthread_exit(NULL);
    return NULL; // non-reaches
}

void test_threads()
{
    struct thread_server server;
    init_server(&server, NULL);
    for (int i = 0; i < NTHREADS; i++)
        new_thread(&server.threads, -1 - (i % 2), &server, some_thread_func);
    while (1) {
        assert(!pthread_rwlock_rdlock(&server.threads.lock));
        int check = server.threads.n == NTHREADS/2;
        assert(!pthread_rwlock_unlock(&server.threads.lock));
        if (check)
            break;
        usleep(10000);
    }
    sleep(1);
    CU_ASSERT_EQUAL(server.threads.n, NTHREADS/2);
    join_threads(&server.threads);
    CU_ASSERT_EQUAL(server.threads.n, 0);
    destroy_server(&server);
}

char template[] = "/tmp/test_socket_XXXXXX";
char socket_name[100];
pid_t server_pid;

void start_server(void)
{
    strcpy(socket_name, template);
    int tmp_fd = mkstemp(socket_name);
    assert(tmp_fd != -1);
    close(tmp_fd);
    server_pid = fork();
    if (server_pid == -1) {
        errnomsg("fork()");
        exit(0);
    }
    if (server_pid == 0) { // Child
        struct thread_server server;
        struct lock_db lock_db;
        struct vdb *db = init_lock_db(&lock_db, NULL);
        init_server(&server, db);
        assert(bind_server(&server, socket_name) == 0);
        run_server(&server);
        destroy_server(&server);
        db->destroy(db);
        exit(0);
    }
}

void stop_server()
{
    debug("killing a server %d...", server_pid);
    if (kill(server_pid, SIGTERM))
        errnomsg("kill().");
    // FIXME: valgrind workaround ((
    sleep(1);
    if (waitpid(server_pid, NULL, WNOHANG) == -1) {
        errnomsg("waitpid().");
        kill(server_pid, SIGKILL);
    }
}

int init(void)
{
    start_server();
    return 0;
}

int clean(void)
{
    stop_server();
    return 0;
}

void client_tests(char *socket_name)
{
    char *tests[][2] = {
        {"=aaa AAAAA\r\n", "\r\n"},
        {"?aaa\r\n", "AAAAA\r\n"},
        {"-aaa\r\n", "\r\n"},
        {"?aaa\r\n", "\r\n"},
        {"=aaa A\r\n", "\r\n"},
        {"=bbb BBB\r\n", "\r\n"},
        {"=ccc CCC\r\n", "\r\n"},
        {"?aaa\r\n", "A\r\n"},
        {"?ccc\r\n", "CCC\r\n"},
        {"?bbb\r\n", "BBB\r\n"},
    };
    struct buffer buf;
    init_buffer(&buf, 1024);
    int sock = socket(PF_LOCAL, SOCK_STREAM, 0);
    CU_ASSERT(sock != -1);
    struct sockaddr_un address = {.sun_family = AF_LOCAL};
    strcpy(address.sun_path, socket_name);
    while (connect(sock, (struct sockaddr *) &address, sizeof(address))) {
        debug("Retry connection...");
        usleep(10000);
    }
    for (size_t i = 0; i < lengthof(tests); i++) {
        ssize_t len = strlen(tests[i][0]);
        ssize_t nsent = send_all(sock, tests[i][0], len);
        CU_ASSERT_EQUAL(nsent, len);
        if (nsent == -1) {
            debug("Client send().");
            break;
        }
        ssize_t nrecv = recv_until(sock, &buf, TELNET_SUFFIX);
        assert(0 < nrecv);
        CU_ASSERT_EQUAL(nrecv, (ssize_t) strlen(tests[i][1]));
        char *ans = buffer_cut(&buf, nrecv);
        debug("answer %d: [%s]", (int) i, ans);
        CU_ASSERT_STRING_EQUAL(ans, tests[i][1]);
        free(ans);
    }
    debug("Client closes a socket...");
    safe_close(sock);
    destroy_buffer(&buf);
}

void test_single_client(void)
{
    client_tests(socket_name);
}

struct thread_data {
    char *socket_name;
};

void *thread_tests(void *arg)
{
    char *tests[][2] = {
        {"=aaa AAAAA\r\n", "\r\n"},
        {"=bbb BBBBB\r\n", "\r\n"},
        {"=ccc CCCCC\r\n", "\r\n"},
        {"?aaa\r\n", "AAAAA\r\n"},
        {"?ccc\r\n", "CCCCC\r\n"},
        {"?bbb\r\n", "BBBBB\r\n"},
    };
    struct buffer buf;
    init_buffer(&buf, 128);
    struct thread_data *thread_data = arg;
    int sock = socket(PF_LOCAL, SOCK_STREAM, 0);
    MUTEX(CU_ASSERT(sock != -1));
    struct sockaddr_un address = {.sun_family = AF_LOCAL};
    strcpy(address.sun_path, thread_data->socket_name);
    while (connect(sock,(struct sockaddr *) &address, sizeof(address)))
        usleep(10000);
    for (size_t i = 0; i < lengthof(tests); i++) {
        ssize_t to_sent = strlen(tests[i][0]);
        ssize_t nsent = send_all(sock, tests[i][0], to_sent);
        assert(nsent == to_sent);
        MUTEX(CU_ASSERT_EQUAL(nsent, to_sent));
        ssize_t nrecv = recv_until(sock, &buf, TELNET_SUFFIX);
        assert(nrecv > 0);
        MUTEX(CU_ASSERT_EQUAL(nrecv, (ssize_t) strlen(tests[i][1])));
        char *rcv = buffer_cut(&buf, nrecv);
        MUTEX(CU_ASSERT_STRING_EQUAL(rcv, tests[i][1]));
        free(rcv);
    }
    safe_close(sock);
    destroy_buffer(&buf);
    pthread_exit(NULL);
    return NULL; // non-reaches
}

#define NCLIENTS 200

void test_clients(void)
{
    pthread_t pthreads[NCLIENTS];
    struct thread_data thread_data[NCLIENTS];
    for (int i = 0; i < NCLIENTS; i++) {
        thread_data[i].socket_name = socket_name;
        pthread_create(pthreads + i, NULL, thread_tests, thread_data + i);
    }
    for (int i = 0; i < NCLIENTS; i++)
        pthread_join(pthreads[i], NULL);
}

int main(void)
{
    int r = EXIT_SUCCESS;
    if (CU_initialize_registry() != CUE_SUCCESS)
        goto exit;
    CU_pSuite suite;
    if (!(suite = CU_add_suite("Threads", NULL, NULL)))
        goto cleanup;
    if (!CU_add_test(suite, "join all threads", test_simple_threads))
        goto cleanup;
    if (!CU_add_test(suite, "join unfinished threads", test_threads))
        goto cleanup;
    if (!(suite = CU_add_suite("Single client", init, clean)))
        goto cleanup;
    if (!CU_add_test(suite, "server + client", test_single_client))
        goto cleanup;
    if (!(suite = CU_add_suite("Multiple clients", init, clean)))
        goto cleanup;
    if (!CU_add_test(suite, "server + clients", test_clients))
        goto cleanup;
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    if (CU_get_failure_list())
        r = EXIT_FAILURE;
cleanup:
    CU_cleanup_registry();
exit:
    if (CU_get_error())
        return CU_get_error();
    else
        return r;
}

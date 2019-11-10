#include <assert.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>

#include <CUnit/Basic.h>

#include "cmds.h"
#include "dblib.h"
#include "errors.h"
#include "fd_utils.h"
#include "misc_macros.h"
#include "poll_server.h"
#include "test_out.h"
#include "telnet_io.h"
#include "telnet_msg.h"

#define NCLIENTS 100

char template[] = "/tmp/test_socket_XXXXXX";
char socket_name[100];
pid_t server_pid;

void start_server(int max_cnct)
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
        struct poll_server server;
        assert(create_db);
        struct vdb *db = create_db(async_response);
        assert(db);
        assert(init_server(&server, max_cnct, db) != -1);
        assert(bind_server(&server, socket_name) == 0);
        run_server(&server);
        destroy_server(&server);
        exit(0);
    }
}

int init_client(void)
{
    start_server(1);
    return 0;
}

int old_err = -1;

void redir_err(void)
{
    old_err = dup(2);
    assert(dup2(open("/dev/null", O_WRONLY), 2) != -1);
}

int init_client_nostderr(void)
{
    redir_err();
    return init_client();
}

int clean_stderr(void)
{
    assert(dup2(old_err, 2) != -1);
    int clean(void);
    return clean();
}

int init_nclients(void)
{
    start_server(NCLIENTS);
    return 0;
}

int init_no_clients(void)
{
    redir_err();
    start_server(0);
    return 0;
}

int clean(void)
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
    return 0;
}

#define NFDS 3

void test_fds(void)
{
    int sockets[2];
    assert(!socketpair(AF_UNIX, SOCK_STREAM, 0, sockets));
    struct poll_server server;
    assert(init_server(&server, NFDS, NULL) != -1);
    CU_ASSERT_EQUAL(add_fd(&server, sockets[0]), 0);
    CU_ASSERT_EQUAL(add_fd(&server, sockets[0]), 1);
    CU_ASSERT_EQUAL(add_fd(&server, sockets[1]), 2);
    CU_ASSERT_EQUAL(cnct_state(CNCT(&server, 1)), CNCT_OPEN);
    remove_connection(&server, 1);
    CU_ASSERT_EQUAL(cnct_state(CNCT(&server, 1)), CNCT_SHUT);
    CU_ASSERT_EQUAL(add_fd(&server, sockets[1]), 1);
    CU_ASSERT_EQUAL(add_fd(&server, sockets[1]), -1);
    CU_ASSERT_EQUAL(add_fd(&server, sockets[1]), -1);
    remove_connection(&server, 2);
    CU_ASSERT_EQUAL(add_fd(&server, sockets[1]), 2);
    destroy_server(&server);
}

void test_answers(void)
{
    struct poll_server server;
    assert(init_server(&server, 2, NULL) != -1);
    int nfd = 1;
    int link[2];
    assert(!socketpair(AF_UNIX, SOCK_STREAM, 0, link));
    FD(&server, nfd) = link[1];
    reset_poll_cnct(CNCT(&server, nfd));
    int client_fd = link[0];
    char *test_line = "test string";
    send_answer(server.cncts + nfd, test_line);
    struct answer req;
    ssize_t rdlen = read_all(server.controlfd->fd, &req, sizeof(req));
    assert(sizeof(req) == rdlen);
    CU_ASSERT_PTR_EQUAL(req.value, test_line);
    //
    inc_queue_cnt(CNCT(&server, nfd));
    async_response(NULL, test_line, server.cncts + nfd);
    rdlen = read_all(server.controlfd->fd, &req, sizeof(req));
    assert(sizeof(req) == rdlen);
    CU_ASSERT_PTR_EQUAL(req.value, test_line);
    CU_ASSERT_EQUAL(req.nfd, nfd);
    CU_ASSERT(FD(&server, req.nfd) != -1);
    dec_queue_cnt(CNCT(&server, nfd));
    inc_queue_cnt(CNCT(&server, nfd));
    async_response(NULL, strdup(test_line), server.cncts + nfd);
    server.controlfd->revents = POLLIN;
    process_control(&server);
    CU_ASSERT(server.fds[nfd].events == (POLLIN | POLLOUT));
    server.fds[nfd].revents = POLLOUT;
    process_client(&server, nfd);
    CU_ASSERT(is_wrbuffer_empty(server.wrbufs + nfd));
    char buf[1024];
    ssize_t nrecv = safe_recv(client_fd, buf, sizeof(buf));
    CU_ASSERT_EQUAL(nrecv, (ssize_t) strlen(test_line) + TELNET_SUFFIX_LEN);
    buf[nrecv - TELNET_SUFFIX_LEN] = 0;
    CU_ASSERT_STRING_EQUAL(buf, test_line);
    //
    close(client_fd);
    destroy_server(&server);
}

void test_simple_client(void)
{
    int link[2];
    assert(!socketpair(AF_UNIX, SOCK_STREAM, 0, link));
    int client_socket = link[0];
    struct vdb *db = create_db(async_response);
    db->set(db, "a", "AAA");
    struct poll_server server;
    assert(init_server(&server, 3, db) != -1);
    int nfd = add_fd(&server, link[1]);
    assert(nfd == 0);
    char *cmd = "?a"TELNET_SUFFIX;
    assert(send_all(client_socket, cmd, strlen(cmd)) ==
           (ssize_t) strlen(cmd));
    poll_fds(&server); // client: read
    poll_fds(&server); // control: read
    poll_fds(&server); // client: write
    struct buffer buf;
    assert(init_buffer(&buf, 1024) != -1);
    ssize_t nrecv = recv_until(client_socket, &buf, TELNET_SUFFIX);
    CU_ASSERT_EQUAL(nrecv, 5);
    char *rcvd = buffer_cut(&buf, nrecv);
    CU_ASSERT_STRING_EQUAL(rcvd, "AAA"TELNET_SUFFIX);
    free(rcvd);
    destroy_buffer(&buf);
    destroy_server(&server);
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
    assert(init_buffer(&buf, 1024) != -1);
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
        if (nsent == -1) {
            errnomsg("client send()");
            break;
        }
        CU_ASSERT_EQUAL(nsent, len);
        ssize_t nrecv = recv_until(sock, &buf, TELNET_SUFFIX);
        assert(nrecv != -1);
        CU_ASSERT_EQUAL(nrecv, (ssize_t) strlen(tests[i][1]));
        char *ans = buffer_cut(&buf, nrecv);
        CU_ASSERT_STRING_EQUAL(ans, tests[i][1]);
        free(ans);
    }
    debug("Client closes a socket...");
    close(sock);
    destroy_buffer(&buf);
}

void test_single_client(void)
{
    client_tests(socket_name);
}

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

#define MUTEX(assertion) do { \
    assert(!pthread_mutex_lock(&mutex)); \
    assertion; \
    assert(!pthread_mutex_unlock(&mutex));\
} while (0)

struct thread_data {
    char *socket_name;
};

int loses;

void *thread_tests(void *arg)
{
    char *tests[][2] = {
        {"=aaa AAAAA"TELNET_SUFFIX, ""TELNET_SUFFIX},
        {"=bbb BBBBB"TELNET_SUFFIX, ""TELNET_SUFFIX},
        {"=ccc CCCCC"TELNET_SUFFIX, ""TELNET_SUFFIX},
        {"?aaa"TELNET_SUFFIX, "AAAAA"TELNET_SUFFIX},
        {"?ccc"TELNET_SUFFIX, "CCCCC"TELNET_SUFFIX},
        {"?bbb"TELNET_SUFFIX, "BBBBB"TELNET_SUFFIX},
    };
    struct buffer buf;
    assert(init_buffer(&buf, 128) != -1);
    struct thread_data *thread_data = (struct thread_data *) arg;
    int sock = socket(PF_LOCAL, SOCK_STREAM, 0);
    MUTEX(CU_ASSERT(sock != -1));
    struct sockaddr_un addr = {.sun_family = AF_LOCAL};
    strcpy(addr.sun_path, thread_data->socket_name);
    while (connect(sock, (struct sockaddr *) &addr, sizeof(addr)))
        usleep(10000);
    for (size_t i = 0; i < lengthof(tests); i++) {
        ssize_t to_sent = strlen(tests[i][0]);
        ssize_t nsent = send_all(sock, tests[i][0], to_sent);
        if (nsent == -1) {
            debug("Client send().");
            MUTEX(loses++);
            goto exit;
        }
        MUTEX(CU_ASSERT_EQUAL(nsent, to_sent));
        ssize_t nrecv = recv_until(sock, &buf, TELNET_SUFFIX);
        if (nrecv == -1) {
            debug("Client recv().");
            MUTEX(loses++);
            goto exit;
        }
        MUTEX(CU_ASSERT_EQUAL(nrecv, (ssize_t) strlen(tests[i][1])));
        if (nrecv > 0) {
            char *rcv = buffer_cut(&buf, nrecv);;
            MUTEX(CU_ASSERT_STRING_EQUAL(rcv, tests[i][1]));
            free(rcv);
        }
    }
    close(sock);
exit:
    destroy_buffer(&buf);
    pthread_exit(NULL);
    return NULL; // non-reaches
}

void test_multiple_clients(int nclients)
{
    pthread_t pthreads[nclients];
    struct thread_data thread_data[nclients];
    for (int i = 0; i < nclients; i++) {
        thread_data[i].socket_name = socket_name;
        pthread_create(pthreads + i, NULL, thread_tests, thread_data + i);
    }
    for (int i = 0; i < nclients; i++)
        pthread_join(pthreads[i], NULL);
}


void test_clients_wo_loses(void)
{
    loses = 0;
    test_multiple_clients(NCLIENTS);
    CU_ASSERT(loses == 0);
}

void test_clients_w_loses(void)
{
    loses = 0;
    test_multiple_clients(NCLIENTS);
    CU_ASSERT(loses > 0);
}

void test_refuses(void)
{
    loses = 0;
    test_multiple_clients(10);
    CU_ASSERT(loses == 10);
}

void test_fail(void)
{
    struct poll_server server;
    char s[1024];
    REDIRECT_OUTPUTS;
    CU_ASSERT(init_server(&server, INT_MAX, NULL) == -1);
    READ_ERR(s);
    CU_ASSERT(strstr(s, "alloc(") != NULL);
    RESTORE_OUTPUTS;
}

int main(int argc, char *argv[])
{
    assert(argc == 2);
    assert(!load_db_lib(argv[1]));
    int r = EXIT_SUCCESS;
    if (CU_initialize_registry() != CUE_SUCCESS)
        goto exit;
    CU_pSuite suite;
//  TODO: does not work on FreeBSD (
/*    if (!(suite = CU_add_suite("Server init. fail", NULL, NULL)))
        goto cleanup;
    if (!CU_add_test(suite, "out of memory", test_fail))
        goto cleanup; */
    if (!(suite = CU_add_suite("DB server commands", NULL, NULL)))
        goto cleanup;
    if (!CU_add_test(suite, "connections", test_fds))
        goto cleanup;
    if (!CU_add_test(suite, "send requests", test_answers))
        goto cleanup;
    if (!CU_add_test(suite, "simple client", test_simple_client))
        goto cleanup;
    if (!(suite = CU_add_suite("Server + single client", init_client, clean)))
        goto cleanup;
    if (!CU_add_test(suite, "run a client", test_single_client))
        goto cleanup;
    if (!(suite = CU_add_suite("Server + multiple clients", init_nclients, clean)))
        goto cleanup;
    if (!CU_add_test(suite, "run clients", test_clients_wo_loses))
        goto cleanup;
    if (!(suite = CU_add_suite("Server loses clients", init_client_nostderr, clean_stderr)))
        goto cleanup;
    if (!CU_add_test(suite, "run clients", test_clients_w_loses))
        goto cleanup;
    if (!(suite = CU_add_suite("Server always refuses", init_no_clients, clean_stderr)))
        goto cleanup;
    if (!CU_add_test(suite, "run clients", test_refuses))
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

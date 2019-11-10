#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include <CUnit/Basic.h>

#include "errors.h"
#include "test_out.h"
#include "fd_utils.h"

int signal_catched;

void sig_handler(int signal)
{
    signal_catched = 1;
}

#define BUF_SIZE (128 *1024)

char line[BUF_SIZE];

int init(void)
{
    struct sigaction sa = {
        .sa_flags=0,
        .sa_handler = sig_handler
    };
    sigemptyset(&sa.sa_mask);
    assert(!sigaction(SIGTERM, &sa, NULL));
    for (size_t i = 0; i < sizeof(line); i++)
        line[i] = i % 26 + 'a';
    line[sizeof(line) - 1] = '\0';
    return 0;
}

void test_bind(void)
{
    char s[1024];
    REDIRECT_OUTPUTS;
    CU_ASSERT_EQUAL(create_binded_socket("non-existing/file/name"), -1);
    READ_ERR(s);
    CU_ASSERT(strstr(s, "bind()") != NULL);
    //
    CU_ASSERT_EQUAL(create_binded_socket("/root/no-access"), -1);
    READ_ERR(s);
    CU_ASSERT(strstr(s, "unlink()") != NULL || strstr(s, "bind()") != NULL);
    //
    CU_ASSERT_EQUAL(create_binded_socket("this_file_name_is_to"
            "ooooooooooooooooooooooooooooooooooooooooooooooooo"
            "ooooooooooooooooooooooooooooooooooooooooooooooooo"
            "ooooooooooooooooooooooooooooooooooooooooooooooooo"
            "ooooooooooooooooooooooooooooooooooooooooooooooooo"
            "ooooooooooooooooooooooooooooooooooooooooooooooooo"
            "ooooooooooooooooooooooooooooooooooooooooooooooooo"
            "o_long"),
        -1);
    READ_ERR(s);
    CU_ASSERT(strstr(s, "too long") != NULL);
    int socket = create_binded_socket("socket_file");
    CU_ASSERT(socket >= 0);
    close(socket);
    unlink("socket_file");
    RESTORE_OUTPUTS;
}

void *discard_server(void *arg)
{
    char *socket_name = arg;
    int socket = create_binded_socket(socket_name);
    assert(socket >= 0);
    int client = accept(socket, NULL, 0);
    char buf[1024];
    ssize_t rcv = 0;
    while (1) {
        ssize_t l = safe_recv(client, buf, sizeof(buf));
        if (l > 0)
            rcv += l;
        else
            break;
    }
    close(socket);
    unlink(socket_name);
    intptr_t res = rcv;
    assert(res == rcv);
    pthread_exit((void *) res);
    return NULL; // non-reaches
}

#define BUF_M_SIZE 16

char bigbuf[BUF_M_SIZE *1024 *1024];

void test_send_all(void)
{
    char *socket_name = "send_socket";
    pthread_t server;
    assert(!pthread_create(&server, NULL, discard_server, socket_name));
    int sock = socket(PF_LOCAL, SOCK_STREAM, 0);
    assert(sock != -1);
    struct sockaddr_un addr = {.sun_family = AF_LOCAL};
    strcpy(addr.sun_path, socket_name);
    while (connect(sock, (struct sockaddr *) &addr, sizeof(addr)))
        usleep(1000);
    ssize_t res = send_all(sock, bigbuf, sizeof(bigbuf));
    CU_ASSERT_EQUAL(res, sizeof(bigbuf));
    close(sock);
    intptr_t rcv;
    pthread_join(server, (void**) &rcv);
    CU_ASSERT_EQUAL(res, rcv);
}

void test_safe_send(void)
{
    // TODO:
}

void test_safe_recv(void)
{
    char test[] = "test string";
    signal_catched = 0;
    char buf[BUF_SIZE];
    int link[2];
    assert(!socketpair(AF_UNIX, SOCK_STREAM, 0, link));
    pid_t pid = fork();
    assert(pid >= 0);
    if (!pid) {
        sleep(1); // FIXME: RACES!
        kill(getppid(), SIGTERM);
        sleep(1); // FIXME: races!
        assert(send_all(link[1], test, sizeof(test)) == sizeof(test));
        sleep(1);
        assert(send_all(link[1], test, sizeof(test)) == sizeof(test));
        exit(0);
    }
    CU_ASSERT(safe_recv(link[0], buf, sizeof(buf)) == 0); // EINITR
    CU_ASSERT(signal_catched); // TODO: Q. Is barrier required here?
    CU_ASSERT(safe_recv(link[0], buf, sizeof(buf)) == sizeof(test));
    CU_ASSERT(!strncmp(buf, test, sizeof(test)));
    /* FIXME: non-blocking tests?
    CU_ASSERT_EQUAL(set_nonblock(link[0]), 0);
    CU_ASSERT_EQUAL(safe_recv(link[0], buf, sizeof(buf)), 0);
    size_t len = 0;
    while (len < sizeof(test)) {
        ssize_t nrecv =  safe_recv(link[0], buf + len, sizeof(buf) - len);
        debug("nrecv = %d", (int) nrecv);
        CU_ASSERT(nrecv >= 0);
        len += nrecv;
    }
    CU_ASSERT_EQUAL(len, sizeof(test));
    CU_ASSERT(!strncmp(buf, test, sizeof(test)));
    CU_ASSERT_EQUAL(safe_recv(link[0], buf, sizeof(buf)), 0);
    */
    close(link[0]);
    close(link[1]);
}

void test_safe_close(void)
{
    char s[1024], test[1024];
    REDIRECT_OUTPUTS;
    int link[2];
    assert(!pipe(link));
    CU_ASSERT(safe_close(link[0]) == 0);
    CU_ASSERT(safe_close(link[1]) == 0);
    CU_ASSERT(safe_close(link[0]) == -1);
    READ_ERR(s);
    sprintf(test, "close(%d)", link[0]);
    CU_ASSERT(strstr(s, test) != NULL)
    RESTORE_OUTPUTS;
}

int main()
{
    int r = EXIT_SUCCESS;
    if (CU_initialize_registry() != CUE_SUCCESS)
        goto exit;
    CU_pSuite suite = CU_add_suite("Socket utils", init, NULL);
    if (!suite)
        goto cleanup;
    if (!CU_add_test(suite, "socket binding", test_bind))
        goto cleanup;
    if (!CU_add_test(suite, "send all", test_send_all))
        goto cleanup;
    if (!CU_add_test(suite, "safe send", test_safe_send))
        goto cleanup;
    if (!CU_add_test(suite, "safe recv", test_safe_recv))
        goto cleanup;
    if (!CU_add_test(suite, "safe close", test_safe_close))
        goto cleanup;
    // TODO: read_all & write_all tests
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

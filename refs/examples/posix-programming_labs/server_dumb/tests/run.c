#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>

#include <CUnit/Basic.h>

#include "db_simple.h"
#include "dumb_server.h"
#include "errors.h"
#include "fd_utils.h"
#include "misc_macros.h"
#include "test_out.h"
#include "telnet_io.h"
#include "telnet_msg.h"

char socket_name[100];
pid_t server_pid;

/// Sets testing suit up.
int init(void)
{
    char *template = "/tmp/test_socket_XXXXXX";
    strcpy(socket_name, template);
    int tmp_fd = mkstemp(socket_name);
    assert(tmp_fd != -1);
    assert(close(tmp_fd) != -1);
    server_pid = fork();
    assert(server_pid != -1);
    if (server_pid == 0) {
        struct simple_db simple_db;
        struct vdb *db = init_simple_db(&simple_db, NULL);
        assert(db);
        int binded_socket = create_binded_socket(socket_name);
        assert(binded_socket != -1);
        run_server(binded_socket, db);
        db->destroy(db);
        exit(EXIT_SUCCESS);
    }
    return 0;
}

int clean(void)
{
    debug("killing a server %d...", server_pid);
    if (kill(server_pid, SIGTERM))
        errnomsg("kill()");
    if (waitpid(server_pid, NULL, 0) == -1)
        errnomsg("waitpid()");
    return 0;
}

void client_tests()
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
    init_buffer(&buf, 128);
    int sock = socket(PF_LOCAL, SOCK_STREAM, 0);
    assert(sock != -1);
    struct sockaddr_un address = {.sun_family = AF_LOCAL};
    strcpy(address.sun_path, socket_name);
    while (connect(sock, (struct sockaddr *) &address, sizeof(address)))
        usleep(10000);
    for (size_t i = 0; i < lengthof(tests); i++) {
        ssize_t sent = strlen(tests[i][0]);
        ssize_t nsent = send_all(sock, tests[i][0], sent);
        CU_ASSERT_EQUAL(nsent, sent);
        ssize_t nrcvd = recv_until(sock, &buf, TELNET_SUFFIX);
        CU_ASSERT_EQUAL(nrcvd, (ssize_t) strlen(tests[i][1]));
        char *received = buffer_cut(&buf, nrcvd);
        CU_ASSERT_STRING_EQUAL(received, tests[i][1]);
        free(received);
    }
    destroy_buffer(&buf);
    close(sock);
}

#define NCLIENTS 1

void test_clients(void)
{
    for (int i = 0; i < NCLIENTS; i++)
        client_tests();
}

void test_intr_server_on_recv(void)
{
    char s[1024], test[1024];
    REDIRECT_OUTPUTS;
    char *cmd_set = "=a AAA\r\n";
    char *cmd_get = "?a\r\n";
    int sock = socket(PF_LOCAL, SOCK_STREAM, 0);
    assert(sock != -1);
    struct sockaddr_un address = {.sun_family = AF_LOCAL};
    strcpy(address.sun_path, socket_name);
    while (connect(sock, (struct sockaddr *) &address, sizeof(address)))
        usleep(10000);
    char buf[1024];
    ssize_t l = send_all(sock, cmd_set, strlen(cmd_set));
    assert(l == (ssize_t) strlen(cmd_set));
    l = safe_recv(sock, buf, sizeof(buf));
    assert(l == 2);
    sleep(1); // FIXME: is 1 sec enough to process client request?
    debug("killing a server %d...", server_pid);
    assert(kill(server_pid, SIGTERM) != -1);
    sleep(1); // FIXME: is 1 sec enough to deliver a signal to the server?
    l = send_all(sock, cmd_get, strlen(cmd_get));
    CU_ASSERT_EQUAL(l, -1);
    READ_ERR(s);
    sprintf(test, "send(%d)", sock);
    CU_ASSERT(strstr(s, test) != NULL);
    RESTORE_OUTPUTS;
    assert(safe_close(sock) != -1);
    assert(waitpid(server_pid, NULL, 0) != -1);
}

void test_intr_server_on_send(void)
{
    char s[1024], test[1024];
    REDIRECT_OUTPUTS;
    char *cmd_set = "=Z ZZZZ\r\n";
    char *cmd_get = "?Z\r\n";
    int sock = socket(PF_LOCAL, SOCK_STREAM, 0);
    assert(sock != -1);
    struct sockaddr_un address = {.sun_family = AF_LOCAL};
    strcpy(address.sun_path, socket_name);
    while (connect(sock, (struct sockaddr *) &address, sizeof(address)))
        usleep(10000);
    char buf[1024];
    ssize_t l;
    l = strlen(cmd_set);
    assert(send_all(sock, cmd_set, l) == l);
    assert(read_all(sock, buf, TELNET_SUFFIX_LEN) == TELNET_SUFFIX_LEN);
    l = strlen(cmd_get);
    assert(send_all(sock, cmd_get, l) == l);
    sleep(1); // FIXME: is 1 sec enough to deliver a command to the server?
    debug("Terminating a server %d...", server_pid);
    assert(!kill(server_pid, SIGTERM));
    sleep(1); // FIXME: is 1 sec enough to deliver a signal to the server?
    int cnt = 0;
    debug("receiving...");
    do {
        ssize_t nrecv = safe_recv(sock, buf, sizeof(buf));
        assert(nrecv > 0);
        cnt += nrecv;
    } while (buf[cnt - 1] != TELNET_SUFFIX[1]);
    sleep(1); // FIXME: is 1 sec enough to shutdown a server?
    l = send_all(sock, cmd_get, strlen(cmd_get));
    CU_ASSERT_EQUAL(l, -1);
    READ_ERR(s);
    sprintf(test, "send(%d)", sock);
    CU_ASSERT(strstr(s, test) != NULL);
    RESTORE_OUTPUTS;
    assert(safe_close(sock) != -1);
    assert(waitpid(server_pid, NULL, 0) != -1);
}

int main()
{
    int r = EXIT_SUCCESS;
    if (CU_initialize_registry() != CUE_SUCCESS)
        goto exit;
    CU_pSuite suite;
    if (!(suite = CU_add_suite("using a server", init, clean)))
        goto cleanup;
    if (!CU_add_test(suite, "run clients", test_clients))
        goto cleanup;
    if (!(suite = CU_add_suite("stopping a server (1)", init, NULL)))
        goto cleanup;
    if (!CU_add_test(suite, "...on recv", test_intr_server_on_recv))
        goto cleanup;
    if (!(suite = CU_add_suite("stopping a server (2)", init, NULL)))
        goto cleanup;
    if (!CU_add_test(suite, "...on send", test_intr_server_on_send))
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

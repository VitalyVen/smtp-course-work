#include <assert.h>
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
#include "fd_utils.h"
#include "misc_macros.h"
#include "telnet_io.h"

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
    line[sizeof(line) / 2] = '\r';
    line[sizeof(line) / 2 + 1] = '\n';
    return 0;
}

void test_recv_until_simple(void)
{
    int link[2];
    assert(!socketpair(AF_UNIX, SOCK_STREAM, 0, link));
    char *test = "=aaa AAAAA\r\n";
    struct buffer buf;
    init_buffer(&buf, 1024);
    ssize_t nsend = strlen(test);
    assert(send_all(link[1], test, nsend) == nsend);
    ssize_t nrecv = recv_until(link[0], &buf, "\r\n");
    CU_ASSERT_EQUAL(nrecv, nsend);
    CU_ASSERT(!strncmp(buf.buf, test, nrecv));
    close(link[0]);
    close(link[1]);
    destroy_buffer(&buf);
}

void test_recv_until(void)
{
    signal_catched = 0;
    int link[2];
    assert(!socketpair(AF_UNIX, SOCK_STREAM, 0, link));
    pid_t pid = fork();
    assert(pid >= 0);
    if (!pid) {
        kill(getppid(), SIGTERM);
        sleep(1); // FIXME:
        ssize_t nsend = strlen(line) + 1;
        assert(send_all(link[1], line, nsend) == nsend);
        sleep(1);
        close(link[1]);
        exit(0);
    }
    close(link[1]);
    struct buffer buf;
    init_buffer(&buf, BUF_SIZE);
    ssize_t nrecv = recv_until(link[0], &buf, "\r\n");
    CU_ASSERT_EQUAL(nrecv, BUF_SIZE / 2 + 2);
    CU_ASSERT_EQUAL(buf.buf[nrecv - 1], '\n');
    CU_ASSERT(!strncmp(buf.buf, line, nrecv));
    CU_ASSERT(signal_catched);
    // assert(!buffer_space(&buf)); // TODO: Q: Why it is a wrong assertion.
    buf.buf[nrecv - 1] = 'N';
    nrecv = recv_until(link[0], &buf, "\n");
    CU_ASSERT_EQUAL(nrecv, 0)
    buffer_clean(&buf, 10); // Free some space in buffer.
    nrecv = recv_until(link[0], &buf, "\n");
    CU_ASSERT_EQUAL(nrecv, -1);
    destroy_buffer(&buf);
}

void test_strip(void)
{
    // TODO: Q. Why?
    struct {
        char cmd[30];
        char *result;
    } cmds[] = {
        {"", NULL},
        {"\r", NULL},
        {"\r\n", ""},
        {"aa bb\r\n", "aa bb"},
        {"aa bb\ncc\r\n", "aa bb\ncc"},
        {"a\n", NULL},
        {"\n", NULL},
    };
    for (size_t i = 0; i < lengthof(cmds); i++) {
        int r = strip_telnet_cmd(cmds[i].cmd);
        if (cmds[i].result) {
            CU_ASSERT_EQUAL(r, 0);
            CU_ASSERT_STRING_EQUAL(cmds[i].cmd, cmds[i].result);
        } else
            CU_ASSERT_EQUAL(r, -1);
    }
}

int main()
{
    int r = EXIT_SUCCESS;
    if (CU_initialize_registry() != CUE_SUCCESS)
        goto exit;
    CU_pSuite suite = CU_add_suite("Telnet utils", init, NULL);
    if (!suite)
        goto cleanup;
    if (!CU_add_test(suite, "receive until (1)", test_recv_until_simple))
        goto cleanup;
    if (!CU_add_test(suite, "receive until (2)", test_recv_until))
        goto cleanup;
    if (!CU_add_test(suite, "strip cmds", test_strip))
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

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <CUnit/Basic.h>

#include "cmds.h"
#include "cnct.h"
#include "cnct_io.h"
#include "db_parallel.h"
#include "db_simple.h"
#include "errors.h"
#include "fd_utils.h"
#include "misc_macros.h"
#include "req_queue_cnd.h"
#include "telnet_io.h"
#include "telnet_msg.h"
#include "wrbuffer.h"
#include "wrbuf_io.h"

#define ASSERT_CMD_NULL(cmd) do { \
    int to_send; \
    char *answer = process_cmd(cmd, db, NULL, &to_send); \
    CU_ASSERT_PTR_NULL(answer); \
    CU_ASSERT_EQUAL(to_send, 1); \
} while (0)

#define ASSERT_CMD(cmd, result) do { \
    int to_send; \
    char *answer = process_cmd(cmd, db, NULL, &to_send); \
    assert(answer); \
    CU_ASSERT_EQUAL(to_send, 1); \
    CU_ASSERT_STRING_EQUAL(answer, result); \
    free(answer); \
} while (0)

void test_cmds(void)
{
    struct simple_db sdb;
    struct vdb *db = init_simple_db(&sdb, NULL);
    char cmd_get_a[] = "?a";
    char cmd_remove_a[] = "-a";
    char cmd_set_b[] = "=b second";
    char cmd_get_b[] = "?b";
    char cmd_get_c[] = "?c";
    //
    db->set(db, "a", "first");
    ASSERT_CMD(cmd_get_a, "first");
    ASSERT_CMD_NULL(cmd_remove_a);
    ASSERT_CMD_NULL(cmd_set_b);
    ASSERT_CMD(cmd_get_b, "second");
    ASSERT_CMD_NULL(cmd_set_b);
    ASSERT_CMD_NULL(cmd_get_a);
    ASSERT_CMD_NULL(cmd_get_c);
    db->destroy(db);
}

struct custom_result {
    struct async_arg async_arg;
    int before_get_cnt;
    int async_result_cnt;
    pthread_mutex_t lock;
};

void before_get(struct custom_result *r)
{
    assert(!pthread_mutex_lock(&r->lock));
    r->before_get_cnt += 1;
    assert(!pthread_mutex_unlock(&r->lock));
}

void async_result(char *key, char *value, void *arg)
{
    struct custom_result *r = arg;
    assert(r);
    assert(!pthread_mutex_lock(&r->lock));
    r->async_result_cnt += 1;
    CU_ASSERT_STRING_EQUAL(key, "a");
    CU_ASSERT_STRING_EQUAL(value, "first");
    assert(!pthread_mutex_unlock(&r->lock));
    free(key);
    free(value);
}

void test_async_result(void)
{
    struct custom_result r = {
        .async_arg = {
            .before_async_get = (async_notify_t) before_get,
        },
        .before_get_cnt = 0,
        .async_result_cnt = 0,
        .lock =  PTHREAD_MUTEX_INITIALIZER,
       };
    struct parallel_db pdb;
    struct vdb *db = init_parallel_db(&pdb, create_cnd_queue, async_result);
    db->set(db, "a", "first");
    CU_ASSERT_EQUAL(r.before_get_cnt, 0);
    CU_ASSERT_EQUAL(r.async_result_cnt, 0);
    int to_send;
    process_cmd("?a", db, &r.async_arg, &to_send);
    CU_ASSERT_EQUAL(to_send, 0);
    CU_ASSERT_EQUAL(r.before_get_cnt, 1);
    db->destroy(db);
    CU_ASSERT_EQUAL(r.async_result_cnt, 1);
    // TODO: destroy r.lock mutex?
}
void test_connection(void)
{
    struct cnct cnct;
    assert(!init_fd_cnct(&cnct, -1));
    CU_ASSERT_EQUAL(cnct_state(&cnct), CNCT_SHUT);
    reset_fd_cnct(&cnct, 1);
    CU_ASSERT_EQUAL(cnct_state(&cnct), CNCT_OPEN);
    CU_ASSERT(is_cnct_queue_empty(&cnct));
    inc_queue_cnt(&cnct);
    CU_ASSERT(!is_cnct_queue_empty(&cnct));
    dec_queue_cnt(&cnct);
    CU_ASSERT(is_cnct_queue_empty(&cnct));
    cnct.async_arg.before_async_get(&cnct.async_arg);
    CU_ASSERT_EQUAL(cnct.queue_cnt, 1);
    CU_ASSERT_EQUAL(shutdown_cnct(&cnct), CNCT_WAIT);
    CU_ASSERT_EQUAL(cnct_state(&cnct), CNCT_WAIT);
    dec_queue_cnt(&cnct);
    CU_ASSERT_EQUAL(cnct_state(&cnct), CNCT_SHUT);
    destroy_cnct(&cnct);
}

void write_async_result(char *key, char *value, void *arg)
{
    struct cnct *cnct = arg;
    assert(cnct);
    sleep(1); // TODO: Races?
    ssize_t len = write_all(cnct_fd(cnct), value, strlen(value));
    assert(len == (ssize_t) strlen(value));
    dec_queue_cnt(cnct);
}

void test_write_connection(void)
{
    char tempname[] = "/tmp/test_write_XXXXXX";
    char *value = "first a-value";
    struct parallel_db pdb;
    struct vdb *db = init_parallel_db(&pdb, create_cnd_queue,
                                      write_async_result);
    db->set(db, "a", value);
    int fd = mkstemp(tempname);
    assert(fd >= 0);
    struct cnct cnct;
    assert(!init_fd_cnct(&cnct, fd));
    CU_ASSERT_EQUAL(cnct_state(&cnct), CNCT_OPEN);
    process_cmd("?a", db, &cnct.async_arg, NULL);
    db->destroy(db);
    CU_ASSERT(is_cnct_queue_empty(&cnct));
    assert(!close(cnct_fd(&cnct)));
    destroy_cnct(&cnct);
    fd = open(tempname, O_RDONLY);
    assert(fd >= 0);
    char buf[100];
    // It's a test so EINTR is not checked.
    ssize_t n = read(fd, buf, sizeof(buf));
    assert(n >= 0);
    buf[n] = '\0';
    CU_ASSERT_STRING_EQUAL(buf, value);
    close(fd);
}

void test_pollfd_connection(void)
{
    struct pollfd pollfd;
    pollfd.fd = -1;
    struct cnct cnct;
    assert(!init_poll_cnct(&cnct, &pollfd));
    CU_ASSERT_EQUAL(cnct_state(&cnct), CNCT_SHUT);
    pollfd.fd = 1;
    reset_poll_cnct(&cnct);
    CU_ASSERT_EQUAL(cnct_state(&cnct), CNCT_OPEN);
    inc_queue_cnt(&cnct);
    CU_ASSERT_EQUAL(cnct_state(&cnct), CNCT_OPEN);
    CU_ASSERT_EQUAL(shutdown_cnct(&cnct), CNCT_WAIT);
    dec_queue_cnt(&cnct);
    CU_ASSERT_EQUAL(cnct_state(&cnct), CNCT_SHUT);
    reset_poll_cnct(&cnct);
    CU_ASSERT_EQUAL(cnct_state(&cnct), CNCT_OPEN);
    destroy_cnct(&cnct);
}

void test_requests(void)
{
    char *tests[][2] = {
        {"=aaa AAAAA\r\n", "\r\n"},
        {"?aaa\r\n", "AAAAA\r\n"},
        {"-aaa\r\n", "\r\n"},
        {"?aaa\r\n", "\r\n"},
        {"=aaa AAAAAAA\r\n", "\r\n"},
        {"?aaa\r\n", "AAAAAAA\r\n"},
        {"=aaa B\r\n", "\r\n"},
        {"?aaa\r\n", "B\r\n"},
        {"=ccccc CCC\r\n", "\r\n"},
        {"?ccccc\r\n", "CCC\r\n"},
    };
    struct simple_db sdb;
    struct vdb *db = init_simple_db(&sdb, NULL);
    int link[2];
    if (socketpair(AF_LOCAL, SOCK_STREAM, 0, link))
        errnomsg("socketpair()");
    struct cnct cnct;
    init_fd_cnct(&cnct, link[1]);
    struct buffer buf;
    init_buffer(&buf, 128);
    for (size_t i = 0; i < lengthof(tests); i++) {
        char *req = tests[i][0];
        char *answ = tests[i][1];
        ssize_t nsent = send(link[0], req, strlen(req), 0);
        CU_ASSERT_EQUAL(nsent, (ssize_t) strlen(req));
        int result = process_cnct(&cnct, db);
        CU_ASSERT_NOT_EQUAL(result, -1);
        if (result != -1) {
            ssize_t nrcvd = recv_until(link[0], &buf, TELNET_SUFFIX);
            CU_ASSERT_EQUAL(nrcvd, (ssize_t) strlen(answ));
            char *received = buffer_cut(&buf, nrcvd);
            CU_ASSERT_STRING_EQUAL(received, answ);
            free(received);
        }
    }
    db->destroy(db);
    destroy_cnct(&cnct);
    destroy_buffer(&buf);
}

void send_async_result(char *key, char *value, void *arg)
{
    struct cnct *cnct = arg;
    assert(cnct);
    send_wsuffix(cnct->fd, value, TELNET_SUFFIX);
    dec_queue_cnt(cnct);
}

void     test_async_requests(void)
{
    char *tests[][2] = {
        {"=aaa AAAAA\r\n", "\r\n"},
        {"?aaa\r\n", "AAAAA\r\n"},
        {"-aaa\r\n", "\r\n"},
        {"?aaa\r\n", "\r\n"},
        {"=aaa A\r\n", "\r\n"},
        {"?aaa\r\n", "A\r\n"},
    };
    struct parallel_db pdb;
    struct vdb *db = init_parallel_db(&pdb, create_cnd_queue,
                                      send_async_result);
    int link[2];
    if (socketpair(AF_LOCAL, SOCK_STREAM, 0, link))
        errnomsg("socketpair()");
    struct cnct cnct;
    init_fd_cnct(&cnct, link[1]);
    int client = link[0];
    struct buffer buf;
    init_buffer(&buf, 128);
    for (size_t i = 0; i < lengthof(tests); i++) {
        char *req = tests[i][0];
        char *ans = tests[i][1];
        // Client: request send.
        ssize_t nsent = send_all(client, req, strlen(req));
        CU_ASSERT_EQUAL(nsent, (ssize_t) strlen(req));
        // Server: request processed.
        int result = process_cnct(&cnct, db);
        CU_ASSERT_EQUAL(result, 1);
        // Client: response received.
        ssize_t len = recv_until(client, &buf, TELNET_SUFFIX);
        CU_ASSERT_EQUAL(len, (ssize_t) strlen(ans));
        if (len > 0) {
            char *rcvd = buffer_cut(&buf, len);
            CU_ASSERT_STRING_EQUAL(rcvd, ans);
            free(rcvd);
        }
    }
    while (!is_cnct_queue_empty(&cnct))
        sleep(1); // Waiting for db to shutdown;
    CU_ASSERT(is_cnct_queue_empty(&cnct));
    CU_ASSERT_EQUAL(shutdown_cnct(&cnct), CNCT_SHUT);
    CU_ASSERT_EQUAL(cnct_state(&cnct), CNCT_SHUT);
    db->destroy(db);
    destroy_cnct(&cnct);
    destroy_buffer(&buf);
}

#define BIG_DATA (16 *1024 *1024)
#define NTESTS 3

void test_long_response(void)
{
    struct simple_db sdb;
    struct vdb *db = init_simple_db(&sdb, NULL);
    int link[2];
    assert(!socketpair(AF_LOCAL, SOCK_STREAM, 0, link));
    char *value = malloc(BIG_DATA);
    assert(value);
    for (int i = 0; i < BIG_DATA - 1; i++)
        value[i] = 'a' + i % 26;
    value[BIG_DATA - 1] = '\0';
    assert(!db->set(db, "key", value));
    //
    char *r = db->get(db, "key");
    assert(r);
    CU_ASSERT_STRING_EQUAL(value, r);
    CU_ASSERT_PTR_NOT_EQUAL(value, r);
    free(r);
    //
    pid_t pid = fork();
    if (pid < 0)
        errnomsg("fork()");
    if (pid > 0) { // parent
        char cmd[] = "?key\r\n";
        char *buf = malloc(BIG_DATA + TELNET_SUFFIX_LEN);
        assert(buf);
        for (int n = 0; n < NTESTS; n++) {
            send_all(link[0], cmd, strlen(cmd));
            int pos = 0;
            while (1) {
                int l = safe_recv(link[0], buf + pos, 1024);
                assert(l != -1);
                pos += l;
                if (pos >= TELNET_SUFFIX_LEN)
                    if (!strncmp(buf + pos - TELNET_SUFFIX_LEN,
                        TELNET_SUFFIX, TELNET_SUFFIX_LEN))
                        break;
            }
            CU_ASSERT(pos > TELNET_SUFFIX_LEN );
            buf[pos - TELNET_SUFFIX_LEN] = '\0';
            CU_ASSERT_STRING_EQUAL(value, buf);
        }
        free(buf);
        wait(NULL);
    }
    else {
        struct cnct cnct;
        init_fd_cnct(&cnct, link[1]);
        for (int n = 0; n < NTESTS; n++)
            process_cnct(&cnct, db);
        destroy_cnct(&cnct);
        exit(0);
    }
    db->destroy(db);
}

void test_wrbuffer_io(void)
{
    int link[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, link)) {
        errnomsg("socketpair()");
        return;
    }
    struct wrbuffer buf;
    init_wrbuffer(&buf);
    struct buffer rdbuf;
    init_buffer(&rdbuf, 1024);
    wrbuffer_add(&buf, strdup("ABC"));
    send_wrbuffer(&buf, link[1]);
    wrbuffer_add(&buf, strdup("DEF"));
    wrbuffer_add(&buf, strdup("GHI\n"));
    wrbuffer_add(&buf, strdup("JKL"));
    send_wrbuffer(&buf, link[1]);
    wrbuffer_add(&buf, strdup("MNO\n"));
    send_wrbuffer(&buf, link[1]);
    ssize_t nrecv = recv_until(link[0], &rdbuf, "\n");
    CU_ASSERT_EQUAL(nrecv, 10);
    char *rcvd = buffer_cut(&rdbuf, nrecv);
    CU_ASSERT_STRING_EQUAL(rcvd, "ABCDEFGHI\n");
    free(rcvd);
    nrecv = recv_until(link[0], &rdbuf, "\n");
    CU_ASSERT_EQUAL(nrecv, 7);
    rcvd = buffer_cut(&rdbuf, nrecv);
    CU_ASSERT_STRING_EQUAL(rcvd, "JKLMNO\n");
    free(rcvd);
    close(link[0]);
    close(link[1]);
    destroy_buffer(&rdbuf);
    destroy_wrbuffer(&buf);
}

int main()
{
    int r = EXIT_SUCCESS;
    if (CU_initialize_registry() != CUE_SUCCESS)
        goto exit;
    CU_pSuite suite;
    suite = CU_add_suite("DB server commands", NULL, NULL);
    if (!suite)
        goto cleanup;
    if (!CU_add_test(suite, "commands", test_cmds))
        goto cleanup;
    if (!CU_add_test(suite, "async result", test_async_result))
        goto cleanup;
    suite = CU_add_suite("Async commands", NULL, NULL);
    if (!suite)
        goto cleanup;
    if (!CU_add_test(suite, "queued cnct interface", test_connection))
        goto cleanup;
    if (!CU_add_test(suite, "queued cnct usage", test_write_connection))
        goto cleanup;
    if (!CU_add_test(suite, "queued cnct usage (pollfd)", test_pollfd_connection))
        goto cleanup;
    suite = CU_add_suite("Socket processing", NULL, NULL);
    if (!suite)
        goto cleanup;
    if (!CU_add_test(suite, "sockets (simple db)", test_requests))
        goto cleanup;
    if (!CU_add_test(suite, "sockets (parallel db)", test_async_requests))
        goto cleanup;
    if (!CU_add_test(suite, "sockets (long response)", test_long_response))
        goto cleanup;
    suite = CU_add_suite("Buffer i/o", NULL, NULL);
    if (!suite)
        goto cleanup;
    if (!CU_add_test(suite, "wrbuffer", test_wrbuffer_io))
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

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <CUnit/Basic.h>

#include "errors.h"
#include "fd_utils.h"
#include "logger.h"
#include "misc_macros.h"
#include "process.h"
#include "redir.h"
#include "test_out.h"

#define BUF_SIZE 32


char *msgs[] = {
    "first message\n", "second message\n", "\n", "third one\n",
    "fourth message (very long)\n", "fifth message\n", "\n", "last one\n",
};

unsigned nmsgs;

void process(char *msg)
{
    assert(nmsgs < lengthof(msgs));
    CU_ASSERT_STRING_EQUAL(msgs[nmsgs++], msg);
}

void test_process_buffer(void)
{
    char allmsgs[BUF_SIZE *lengthof(msgs)] = "";
    for (size_t i = 0; i < lengthof(msgs); i++) {
        assert(strlen(msgs[i]) < BUF_SIZE);
        strcat(allmsgs, msgs[i]);
    }
    char buf[BUF_SIZE] = "";
    ssize_t srcpos = 0, srclen = strlen(allmsgs);
    nmsgs = 0;
    while (srcpos <= srclen) {
        ssize_t pos = strlen(buf);
        ssize_t n = BUF_SIZE - pos - 1;
        strncpy(buf + pos, allmsgs + srcpos, n);
        srcpos += n;
        buf[pos + n] = '\0';
        ssize_t in_buff = process_buffer(buf, process);
        CU_ASSERT_EQUAL(in_buff, (ssize_t) strlen(buf));
    }
    CU_ASSERT_EQUAL(nmsgs, lengthof(msgs));
}

struct assertions {
    size_t n;
    char received[50][BUF_SIZE];
};

char *test_msgs[] = {
    "a message\n",
    "second short message\n",
    "\n",
    "third short message\n",
    "\n",
    "close to finish\n",
    "last message\n",
};

void write_test_msgs(int fd)
{
    for (size_t i = 0; i < lengthof(test_msgs); i++)
        write_all(fd, test_msgs[i], strlen(test_msgs[i]));
}

void write_test_fmsgs(FILE *f)
{
    for (size_t i = 0; i < lengthof(test_msgs); i++)
        fprintf(f, "%s", test_msgs[i]);
}

void write_single_test_msg(int fd)
{
    char msg[lengthof(test_msgs) *BUF_SIZE] = "";
    for (size_t i = 0; i < lengthof(test_msgs); i++)
        strcat(msg, test_msgs[i]);
    write_all(fd, msg, strlen(msg));
}

void write_ch_msgs(int fd)
{
    for (size_t i = 0; i < lengthof(test_msgs); i++)
        for (size_t l = strlen(test_msgs[i]), k = 0; k < l; k++)
            write_all(fd, test_msgs[i] + k, 1);
}

//     debug("process_%d: %s", i, msg);

#define process(i) void process_##i(char *msg) \
{ \
    if  (strstr(msg, "[DEBUG]") != NULL) return; \
    strcpy(assertions[i].received[assertions[i].n], msg); \
    assertions[i].n++; \
}

struct assertions *assertions;

process(0)
process(1)
process(2)
process(3)
process(4)

process_t processes[] = {
    process_0, process_1, process_2, process_3, process_4,
};

int init(void)
{
    int id = shmget(IPC_PRIVATE, sizeof(struct assertions) *sizeof(processes),
                    IPC_CREAT | 0666);
    if (id == -1) {
        errnomsg("shmget()");
        return -1;
    }
    assertions = shmat(id, NULL, 0);
    if (assertions == (void *) -1) {
        errnomsg("shmat()");
        return -1;
    }
    return 0;
}

int clean(void)
{
    if (shmdt(assertions)) {
        errnomsg("shmdt()");
        return -1;
    }
    return 0;
}

void reset_assertions(void)
{
    for (size_t k = 0; k < lengthof(processes); k++)
        assertions[k].n = 0;
}

void check_assertions(size_t ninputs, size_t nmsgs)
{
    for (size_t k = 0; k < lengthof(processes); k++)
        if (k >= ninputs) {
            CU_ASSERT_EQUAL(assertions[k].n, 0);
        } else {
            // if (assertions[k].n != nmsgs)
            //     printf("k = %d, a[k].n = %d, nmsgs = %d\n",
            //   file:/         (int) k, (int) assertions[k].n, (int) nmsgs);
            CU_ASSERT_EQUAL(assertions[k].n, nmsgs);
            for (size_t i = 0; i < assertions[k].n; i++)
                // if (strcmp(test_msgs[i % lengthof(test_msgs)],
                //            assertions[k].received[i])) {
                //     printf("t: %s", test_msgs[i % lengthof(test_msgs)]);
                //     printf("a: %s", assertions[k].received[i]);
                // }
                CU_ASSERT_STRING_EQUAL(
                    test_msgs[i % lengthof(test_msgs)],
                    assertions[k].received[i]);
        }
}

void test_process_input(void)
{
    reset_assertions();
    int pipefds[2];
    if (pipe(pipefds)) {
        errnomsg("pipe()");
        return ;
    }
    pid_t pid = fork();
    if (pid < 0) {
        errnomsg("fork()");
        return;
    }
    if (pid > 0) {
        close(pipefds[0]);
        write_test_msgs(pipefds[1]);
        write_single_test_msg(pipefds[1]);
        write_ch_msgs(pipefds[1]);
        close(pipefds[1]);
        if (waitpid(pid, NULL, 0) == -1) {
            errnomsg("waitpid()");
            return;
        }
        check_assertions(1, 3 *lengthof(test_msgs));
    }
    else {
        close(pipefds[1]);
        process_input(pipefds[0], process_0, BUF_SIZE);
        close(pipefds[0]);
        exit(0);
    }
}

void test_poll_inputs(void)
{
    reset_assertions();
    int pipefds[lengthof(processes)][2];
    for (size_t i = 0; i < lengthof(processes); i++)
        assert(!pipe(pipefds[i]));
    pid_t pid = fork();
    assert(pid >= 0);
    if (pid > 0) { // Parent.
        for (size_t p = 0; p < lengthof(processes); p++)
            close(pipefds[p][0]);
        for (size_t m = 0; m < lengthof(test_msgs); m++)
            for (size_t i = 0; i < lengthof(processes); i++)
                write_all(pipefds[i][1], test_msgs[m], strlen(test_msgs[m]));
        for (size_t p = 0; p < lengthof(processes); p++)
            close(pipefds[p][1]);
        debug("waitpid(%d)...", (int) pid);
        if (waitpid(pid, NULL, 0) == -1) {
            errnomsg("waitpid()");
            return;
        }
        check_assertions(lengthof(processes), lengthof(test_msgs));
    }
    else {   // Child.
        int fds[lengthof(processes)];
        for (size_t p = 0; p < lengthof(processes); p++) {
            close(pipefds[p][1]);
            fds[p] = pipefds[p][0];
        }
        poll_inputs(lengthof(processes), fds, processes, BUF_SIZE);
        for (size_t p = 0; p < lengthof(processes); p++)
            close(pipefds[p][0]);
        exit(0);
    }
}

void test_buffer_overrun(void)
{
    REDIRECT_OUTPUTS;
    char *msgs[] = {
        "short message\n", "123456789012345message\n",
        "last message\n", "double trouble very long message\n"
    };
    reset_assertions();
    int pipefds[2];
    if (pipe(pipefds)) {
        errnomsg("pipe()");
        return ;
    }
    pid_t pid = fork();
    assert (pid != -1);
    if (pid > 0) {
        close(pipefds[0]);
        for (size_t i = 0; i < lengthof(msgs); i++) {
            ssize_t len = write_all(pipefds[1], msgs[i], strlen(msgs[i]));
            assert(len == (ssize_t) strlen(msgs[i]));
        }
        close(pipefds[1]);
        if (waitpid(pid, NULL, 0) == -1) {
            errnomsg("waitpid()");
            return;
        }
        CU_ASSERT_EQUAL(assertions[0].n, lengthof(msgs));
        CU_ASSERT_STRING_EQUAL(assertions[0].received[0], msgs[0]);
        CU_ASSERT_STRING_EQUAL(assertions[0].received[1], "...message\n");
        CU_ASSERT_STRING_EQUAL(assertions[0].received[2], msgs[2]);
        CU_ASSERT_STRING_EQUAL(assertions[0].received[3], "...ssage\n");
    }
    else {
        close(pipefds[1]);
        process_input(pipefds[0], process_0, 16);
        close(pipefds[0]);
        exit(0);
    }
    RESTORE_OUTPUTS;
}

void test_fork_redir(void)
{
    reset_assertions();
    char temp_name[] = "/tmp/test_fork_redir_XXXXXX";
    int fd = mkstemp(temp_name);
    if (fd < 0) {
        errnomsg("mkstemp()");
        return;
    }
    FILE *f = fdopen(fd, "w+");
    struct redir r;
    init_redir(&r, process_0, f);
    pid_t pid = fork_redir(&r);
    if (pid < 0)
        return;
    if (pid > 0) {
        debug("writing...");
        write_test_fmsgs(f);
        debug("restoring...");
        restore_redir(&r);
        debug("waiting...");
        if (waitpid(pid, NULL, 0) == -1) {
            errnomsg("waitpid()");
            return;
        }
        debug("checking...");
        check_assertions(1, lengthof(test_msgs));
    }
    else {   // Child
        debug("child has returned.");
        fclose(f);
        exit(0);
    }
    fprintf(f, "test file\n");
    fclose(f);
    FILE *c = fopen(temp_name, "r");
    char s[100];
    fgets(s, sizeof(s), c);
    CU_ASSERT_STRING_EQUAL(s, "test file\n");
    fclose(c);
}

void test_fork_redirs(void)
{
    reset_assertions();
    char temp_names[][50] = {
//         TODO: TO question
        "/tmp/test_fork_redirs1_XXXXXX",
        "/tmp/test_fork_redirs2_XXXXXX",
        "/tmp/test_fork_redirs3_XXXXXX",
    };
    assert(lengthof(temp_names) <= lengthof(processes));
    int fds[lengthof(temp_names)];
    FILE *f[lengthof(temp_names)];
    struct redir r[lengthof(temp_names)];
    for (size_t i = 0; i < lengthof(temp_names); i++) {
        if ((fds[i] = mkstemp(temp_names[i])) < 0) {
            errnomsg("mkstemp()");
            return;
        }
        f[i] = fdopen(fds[i], "w+");
        init_redir(r + i, processes[i], f[i]);
    }
    pid_t pid = fork_redirs(r, lengthof(temp_names));
    if (pid < 0)
        return;
    if (pid > 0) {
        for (size_t i = 0; i < lengthof(temp_names); i++)
            write_test_fmsgs(f[i]);
        for (size_t i = 0; i < lengthof(temp_names); i++) {
            write_test_fmsgs(f[i]);
            restore_redir(r + i);
        }
        if (waitpid(pid, NULL, 0) == -1) {
            errnomsg("waitpid()");
            return;
        }
        check_assertions(lengthof(temp_names), 2 *lengthof(test_msgs));
        for (size_t i = 0; i < lengthof(temp_names); i++)
            fclose(f[i]);
    }
    else {   // Child
        for (size_t i = 0; i < lengthof(temp_names); i++)
            fclose(f[i]);
        exit(0);
    }
}

void test_logger()
{
    reset_assertions();
    struct logger logger;
    logger_init(&logger, process_0, process_1);
    debug("starting...");
    if (logger_start(&logger) <= 0)
        exit(0);
    set_sig_handlers(&logger);
    // fd == 1: stdout
    write_test_msgs(1);
    write_single_test_msg(1);
    write_ch_msgs(1);
    // fd == 2: stderr
    write_test_msgs(2);
    write_single_test_msg(2);
    write_ch_msgs(2);
    clear_sig_handlers();
    logger_stop(&logger);
    check_assertions(2, 3 *lengthof(test_msgs));
}

void test_polling_logger(void)
{
    reset_assertions();
    struct logger logger;
    logger_init(&logger, process_0, process_1);
    if (logger_polled_start(&logger) <= 0)
        exit(0);
    // fd == 1: stdout
    write_test_msgs(1);
    write_single_test_msg(1);
    write_ch_msgs(1);
    // fd == 2: stderr
    write_test_msgs(2);
    write_single_test_msg(2);
    write_ch_msgs(2);
    logger_stop(&logger);
    check_assertions(2, 3 *lengthof(test_msgs));
}

int main()
{
    int r = EXIT_SUCCESS;
    if (CU_initialize_registry() != CUE_SUCCESS)
        goto exit;
    CU_pSuite suite;
    if (!(suite = CU_add_suite("buffers", NULL, NULL)))
        goto cleanup;
    if (!CU_add_test(suite, "process buffer", test_process_buffer))
        goto cleanup;
    if (!(suite = CU_add_suite("input processing", init, clean)))
        goto cleanup;
    if (!CU_add_test(suite, "process input", test_process_input))
        goto cleanup;
    if (!CU_add_test(suite, "process inputs", test_poll_inputs))
        goto cleanup;
    if (!CU_add_test(suite, "buffer overrun", test_buffer_overrun))
        goto cleanup;
    if (!(suite = CU_add_suite("i/o redirection", init, clean)))
        goto cleanup;
    if (!CU_add_test(suite, "fork redir", test_fork_redir))
        goto cleanup;
    if (!CU_add_test(suite, "fork redirs", test_fork_redirs))
        goto cleanup;
    if (!(suite = CU_add_suite("logger", init, clean)))
        goto cleanup;
    if (!CU_add_test(suite, "logger (process per pipe)", test_logger))
        goto cleanup;
    if (!CU_add_test(suite, "logger (repeat)", test_logger))
        goto cleanup;
    if (!CU_add_test(suite, "logger (polling)", test_polling_logger))
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

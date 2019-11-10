#include <assert.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/wait.h>

#include <CUnit/Basic.h>

#include "errors.h"
#include "pidfile.h"
#include "test_out.h"
#include "service.h"

void test_pidfiles(void)
{
    char pidfile[256];
    sprintf(pidfile, "/tmp/test_pidfile_%08ld.pid", (long) getpid());
    char s[1024], test[1024];
    REDIRECT_OUTPUTS;
    CU_ASSERT(check_status(pidfile) == -1);
    READ_ERR(s);
    sprintf(test, "[WARNING] Pid file \"%s\" not found.\n",
            pidfile);
    CU_ASSERT_STRING_EQUAL(s, test);
    CU_ASSERT(open_pidfile(pidfile) == 0);
    CU_ASSERT(open_pidfile(pidfile) == 1);
    CU_ASSERT(close_pidfile() == 0);
    CU_ASSERT(check_status(pidfile) == 0);
    READ_OUT(s);
    sprintf(test, "Service is still running (pid = %d)\n", getpid());
    CU_ASSERT_STRING_EQUAL(s, test);
    CU_ASSERT(delete_pidfile(pidfile) == 0);
    CU_ASSERT(delete_pidfile(pidfile) == -1);
    READ_ERR(s);
    CU_ASSERT(strstr(s, pidfile) != NULL);
    CU_ASSERT(open_pidfile("/invalid/pid/file") == -1);
    RESTORE_OUTPUTS;
}

struct result {
    int value;
    sem_t sem;
};

void test_daemonize(void)
{
    fflush(stdout); // Q. For what purpose?
    int result_id = shmget(IPC_PRIVATE, sizeof(struct result), IPC_CREAT | 0666);
    assert(result_id != -1);
    struct result *result = shmat(result_id, NULL, 0);
    assert(result);
    assert(!sem_init(&result->sem, 1, 0));
    pid_t pid = fork();
    assert(pid != -1);
    if (pid == 0) {
        result->value = 0;
        pid_t old_pid = getpid();
        if (daemonize() == 0)
            result->value |= 1;
        pid_t new_pid = getpid();
        if (new_pid != old_pid)
            result->value |= 2;
        assert(!sem_post(&result->sem));
        exit(0);// exit(result); //TODO: Q. Why it does not work?
    }
    assert(!sem_wait(&result->sem));
    CU_ASSERT_EQUAL(result->value, 3);
    assert(!sem_destroy(&result->sem));
    assert(!shmdt(result));
}

void test_fork_service(void)
{
    char s[1024];
    REDIRECT_OUTPUTS;
    char *argv1[] = {"sh", "-c", "sleep 0", NULL};
    pid_t pid = fork_service(argv1);
    CU_ASSERT(pid > 0);
    CU_ASSERT(wait_service(pid) == 0);
    char *argv2[] = {"/invalid/program", NULL};
    pid = fork_service(argv2);
    CU_ASSERT(wait_service(pid) == -1);
    READ_ERR(s);
    CU_ASSERT(strstr(s, "execvp") != NULL);
    RESTORE_OUTPUTS;
}

char *loop[] = {"sh", "-c", "while true; do sleep 1; done", NULL};

void test_sighandler(void)
{
    char s[1024], test[1024];
    REDIRECT_OUTPUTS;
    pid_t pid = fork_service(loop);
    CU_ASSERT(pid > 0);
    debug("service pid: %d", pid);
    CU_ASSERT(!set_sighandler());
    set_sighandler_pid(pid);
    CU_ASSERT(!kill(getpid(), SIGTERM));
    CU_ASSERT(!wait_service(pid));
    READ_OUT(s);
    sprintf(test, "Signal caught: %d\n", SIGTERM);
    CU_ASSERT_STRING_EQUAL(s, test);
    READ_OUT(s);
    sprintf(test, "Signal %d was sent to process %d\n", SIGTERM, (int) pid);
    CU_ASSERT_STRING_EQUAL(s, test);
    RESTORE_OUTPUTS;
    //printf("s: [%s]\n", s);
}

void test_stop_service(void)
{
    char s[1024];
    REDIRECT_OUTPUTS;
    char pidfile[128];
    sprintf(pidfile, "/tmp/test_stop_%08ld.pid", (long) getpid());
    pid_t pid = fork();
    assert(pid != -1);
    CU_ASSERT_EQUAL(stop_service(pidfile), -1);
    READ_ERR(s);
    CU_ASSERT(strstr(s, pidfile) != NULL);
    if (pid == 0) {
        assert(!open_pidfile(pidfile));
        pid_t service = fork_service(loop);
        assert(service > 0);
        assert(!set_sighandler());
        set_sighandler_pid(service);
        assert(!close_pidfile()); // TODO: Q. Why we cannot do it easlier?
        assert(!wait_service(service));
        assert(!delete_pidfile(pidfile));
        exit(10); // Q. Exit code does matter.
    }
    while (check_status(pidfile)) { // Waiting a service to initinalize,
        READ_ERR(s);
        sleep(1);
    }
    sleep(3);
    CU_ASSERT(!kill(pid, 0)); // Child should be running now.
    pid_t service = stop_service(pidfile);
    CU_ASSERT(service > 0);
    CU_ASSERT_EQUAL(pid, service);
    int status;
    CU_ASSERT(waitpid(pid, &status, 0) != -1);
    CU_ASSERT(WIFEXITED(status));
    CU_ASSERT_EQUAL(WEXITSTATUS(status), 10);
    CU_ASSERT_EQUAL(stop_service(pidfile), -1);
    READ_ERR(s);
    CU_ASSERT(strstr(s, pidfile) != NULL);
    RESTORE_OUTPUTS;
}

void test_service(void)
{
    char s[1024];
    REDIRECT_OUTPUTS;
    int result_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
    assert(result_id != -1);
    int *result = shmat(result_id, NULL, 0);
    assert(result);
    *result = 0;
    char pidfile[128];
    sprintf(pidfile, "/tmp/test_service_%08ld.pid", (long) getpid());
    pid_t pid = fork();
    assert(pid != -1);
    if (pid == 0) { // Child
        assert(!open_pidfile(pidfile));
        daemonize();
        assert(!set_sighandler());
        pid_t service_pid = fork_service(loop);
        assert(service_pid != -1);
        set_sighandler_pid(service_pid);
        assert(!close_pidfile());
        *result = 10;
        assert(!wait_service(service_pid));
        // Q. We cannot check this exit code anyway
        exit(0); // Q. Demonized. Exit code does not matter.
    }
    while (check_status(pidfile)) { // Waiting a service to initinalize,
        READ_ERR(s);
        sleep(1);
    }
    CU_ASSERT(stop_service(pidfile) > 0);
    CU_ASSERT_EQUAL(*result, 10);
    assert(!shmdt(result));
    RESTORE_OUTPUTS;
}

int main()
{
    int r = EXIT_SUCCESS;
    if (CU_initialize_registry() != CUE_SUCCESS)
        goto exit;
    CU_pSuite suite = CU_add_suite("service", NULL, NULL);
    if (!suite)
        goto cleanup;
    if (!CU_add_test(suite, "pidfile test", test_pidfiles))
        goto cleanup;
    if (!CU_add_test(suite, "daemonize test", test_daemonize))
        goto cleanup;
    if (!CU_add_test(suite, "fork service test", test_fork_service))
        goto cleanup;
    if (!CU_add_test(suite, "signal handler test", test_sighandler))
        goto cleanup;
    if (!CU_add_test(suite, "stop service test", test_stop_service))
        goto cleanup;
    if (!CU_add_test(suite, "service test", test_service))
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

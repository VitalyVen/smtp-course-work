#include <assert.h>
#include <stdlib.h>
#include <sys/wait.h>

#include <CUnit/Basic.h>

#ifndef DEBUG
#define DEBUG
#endif

#include "errors.h"
#include "misc_macros.h"
#include "test_out.h"

void test_redirect(void)
{
    REDIRECT_OUTPUTS;
    for (int i = 10000; i < 20000; i+=50) {
        char test[16], s[16];
        sprintf(test, "%d\n", i);
        printf("%s", test);
        READ_OUT(s);
        CU_ASSERT_STRING_EQUAL(s, test);
    }
    for (int i = 10000; i < 20000; i+=50) {
        char test[16], s[16];
        sprintf(test, "%d\n", i);
        fprintf(stderr, "%s", test);
        READ_ERR(s);
        CU_ASSERT_STRING_EQUAL(s, test);
    }
    RESTORE_OUTPUTS;
}

void test_messages(void)
{
    REDIRECT_OUTPUTS;
    char s[1024], test[1024];
    error("invalid: %d", 123);
    READ_ERR(s);
    sprintf(test, "[ERROR][%d][tests/run.c:test_messages:%d] invalid: 123\n",
            (int) getpid(), __LINE__ - 3);
    CU_ASSERT_STRING_EQUAL(s, test);
    debug("(ignored) debug message");
    warning("%s", "suspicious");
    READ_ERR(s);
    CU_ASSERT_STRING_EQUAL(s, "[WARNING] suspicious\n");
    debug("debug message: %d, %s", 123, "fail");
    READ_DEBUG(s);
    sprintf(test, "[DEBUG][%d] debug message: 123, fail\n",
            (int) getpid());
    CU_ASSERT_STRING_EQUAL(s, test);
    RESTORE_OUTPUTS;
}

void test_errnomsg(void)
{
    char s[1024], test[1024];
    REDIRECT_OUTPUTS;
    errno = ENOTDIR;
    sprintf(test, "[ERROR][%d][%s:%s:%d] open(\"%s\", O_RDONLY): %s\n",
            (int) getpid(), __FILE__, __func__, __LINE__ + 2,
            "/notdir/f.txt", strerror(errno));
    errnomsg("open(\"%s\", O_RDONLY)", "/notdir/f.txt");
    READ_ERR(s);
    CU_ASSERT_STRING_EQUAL(s, test);
    RESTORE_OUTPUTS;
}

int errors[] = {EINVAL, EBUSY, EINTR, EAGAIN};
int nerror = 0;

int fail(void)
{
    int r = errors[nerror++];
    if (nerror == lengthof(errors))
        nerror = 0;
    return r;
}

void test_errmsg(void)
{
    char s[1024];
    REDIRECT_OUTPUTS;
    nerror = 0;
    errmsg(fail(), "fail(%s)", "void");
    READ_ERR(s);
    CU_ASSERT(strstr(s, strerror(errors[0])) != NULL);
    CU_ASSERT(strstr(s, "fail(void)") != NULL);
    RESTORE_OUTPUTS;
}

void test_fatal(void)
{
    char s[1024];
    REDIRECT_OUTPUTS;
    pid_t pid = fork();
    assert(pid != -1);
    if (pid == 0) {
        nerror = 0;
        fatal(fail(), "fail(%s)", "void");
        CU_ASSERT(0);
    }
    waitpid(pid, NULL, 0);
    READ_ERR(s);
    CU_ASSERT(strstr(s, strerror(errors[0])) != NULL);
    CU_ASSERT(strstr(s, "fail(void)") != NULL);
    RESTORE_OUTPUTS;
}

void test_checks(void)
{
    REDIRECT_OUTPUTS;
    char s[1024], test[2014];
    int r;
    errno = EINVAL;
    CHECK_EINTR(r = 1, r = 2, "sem_wait()");
    CU_ASSERT_EQUAL(r, 2);
    READ_ERR(s);
    sprintf(test, "[ERROR][%d][%s:%s:%d] sem_wait(): %s\n",
            (int) getpid(), __FILE__,  __func__, __LINE__ - 4,
            strerror(EINVAL));
    CU_ASSERT_STRING_EQUAL(s, test);
    //
    errno = EINTR;
    CHECK_EINTR(r = 1, r = 2, "sem_wait()");
    CU_ASSERT_EQUAL(r, 1);
    READ_DEBUG(s);
    sprintf(test, "[DEBUG][%d] sem_wait(): %s\n",
            (int) getpid(), strerror(EINTR));
    CU_ASSERT_STRING_EQUAL(s, test);
    //
    errno = EINTR;
    CHECK_EINTREAGAIN(r = 1, r =  2, r = 3, "sem_wait()");
    CU_ASSERT_EQUAL(r, 1);
    READ_DEBUG(s);
    sprintf(test, "[DEBUG][%d] sem_wait(): %s\n",
            (int) getpid(), strerror(EINTR));
    CU_ASSERT_STRING_EQUAL(s, test);
    errno = EAGAIN;
    CHECK_EINTREAGAIN(r = 1, r =  2, r = 3, "sem_wait(%d)", 1);
    CU_ASSERT_EQUAL(r, 2);
    READ_DEBUG(s);
    sprintf(test, "[DEBUG][%d] sem_wait(1): %s\n",
            (int) getpid(), strerror(EAGAIN));
    CU_ASSERT_STRING_EQUAL(s, test);
    errno = EINVAL;
    CHECK_EINTREAGAIN(r = 1, r =  2, r = 3, "sem_wait(%d)", 1);
    CU_ASSERT_EQUAL(r, 3);
    RESTORE_OUTPUTS;
}

int main(int argc, char *argv[])
{
    int r = EXIT_SUCCESS;
    if (CU_initialize_registry() != CUE_SUCCESS)
        goto exit;
    CU_pSuite suite;
    if (!(suite = CU_add_suite("Errors", NULL, NULL)))
        goto cleanup;
    if (!CU_add_test(suite, "redirect outputs", test_redirect))
        goto cleanup;
    if (!CU_add_test(suite, "message macross", test_messages))
        goto cleanup;
    if (!CU_add_test(suite, "errnomsg macro", test_errnomsg))
        goto cleanup;
    if (!CU_add_test(suite, "errmsg macro", test_errmsg))
        goto cleanup;
    if (!CU_add_test(suite, "fatal macro", test_fatal))
        goto cleanup;
    if (!CU_add_test(suite, "message functions", test_messages))
        goto cleanup;
    if (!CU_add_test(suite, "CHECK_E* macros", test_checks))
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

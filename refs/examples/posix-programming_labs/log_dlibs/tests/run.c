#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <pthread.h>

#include <CUnit/Basic.h>

#include "loglib.h"
#include "errors.h"

char *log_lib_name;

void test_log_lib(void)
{
    void *log_lib = dlopen(log_lib_name, RTLD_LAZY);
    if (!log_lib) {
        error("%s", dlerror());
        CU_FAIL();
        return;
    };
    const char *(*log_name)() = dlsym(log_lib, "name");
    if (log_name)
        printf("Log library: %s\n", log_name());
    else
        error("%s", dlerror());
    CU_ASSERT_PTR_NOT_NULL(log_name);
    //
    int (*log_start)(char *, char *) = dlsym(log_lib, "start");
    if (!log_start)
        error("%s", dlerror());
    CU_ASSERT_PTR_NOT_NULL(log_start);
    //
    void (*log_stop)(void) = dlsym(log_lib, "stop");
    if (!log_stop)
        error("%s", dlerror());
    CU_ASSERT_PTR_NOT_NULL(log_stop);
    // dlclose() returns 0 on success.
    if (dlclose(log_lib)) {
        error("%s", dlerror());
        CU_FAIL();
    }
}

void usage(char *argv[])
{
    printf("Usage: %s some-log-library.so\n", argv[0]);
    exit(1);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
        usage(argv);
    log_lib_name = argv[1];
    int r = EXIT_SUCCESS;
    if (CU_initialize_registry() != CUE_SUCCESS)
        goto exit;
    CU_pSuite suite = CU_add_suite("Log", NULL, NULL);
    if (!suite)
        goto cleanup;
    if (!CU_add_test(suite, "test log lib", test_log_lib))
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

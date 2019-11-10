#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <CUnit/Basic.h>

#include "log_setup.h"

char *log_lib_name;

void test_log_lib(void)
{
    void *log_lib = setup_log("test", log_lib_name);
    CU_ASSERT_PTR_NOT_NULL(log_lib);
    CU_ASSERT_PTR_NOT_NULL(out_pid);
    CU_ASSERT_PTR_NOT_NULL(err_pid);
    close_log(log_lib);
}

void usage(char *argv[])
{
    printf("Usage: %s some-log-library.so\n", argv[0]);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
        usage(argv);
    int r = EXIT_SUCCESS;
    log_lib_name = argv[1];
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

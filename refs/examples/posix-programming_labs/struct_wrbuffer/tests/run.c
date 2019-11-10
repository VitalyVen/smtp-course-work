#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <CUnit/Basic.h>

#include "wrbuffer.h"

void test_wrbuffer(void)
{
    struct wrbuffer buf;
    init_wrbuffer(&buf);
    wrbuffer_add(&buf, strdup("ABC"));
    wrbuffer_add(&buf, strdup("DEF"));
    CU_ASSERT_EQUAL(wrbuffer_remained(&buf), 3);
    CU_ASSERT_STRING_EQUAL(wrbuffer_start(&buf), "ABC");
    wrbuffer_shift(&buf, 2);
    CU_ASSERT_STRING_EQUAL(wrbuffer_start(&buf), "C");
    CU_ASSERT_EQUAL(wrbuffer_remained(&buf), 1);
    wrbuffer_shift(&buf, 1);
    CU_ASSERT_STRING_EQUAL(wrbuffer_start(&buf), "DEF");
    CU_ASSERT_EQUAL(wrbuffer_remained(&buf), 3);
    CU_ASSERT(!is_wrbuffer_empty(&buf));
    wrbuffer_shift(&buf, 3);
    CU_ASSERT_EQUAL(wrbuffer_remained(&buf), 0);
    CU_ASSERT(is_wrbuffer_empty(&buf));
    destroy_wrbuffer(&buf);
}

int main()
{
    int r = EXIT_SUCCESS;
    if (CU_initialize_registry() != CUE_SUCCESS)
        goto exit;
    CU_pSuite suite = CU_add_suite("buffer", NULL, NULL);
    if (!suite)
        goto cleanup;
    if (!CU_add_test(suite, "buffer tests", test_wrbuffer))
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

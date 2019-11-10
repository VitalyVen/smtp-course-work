#include <assert.h>
#include <limits.h>
#include <stdio.h>

#include <CUnit/Basic.h>

#include "buffer.h"
#include "test_out.h"

void test_buffer(void)
{
    char *test = "aabbcc";
    char *cut;
    struct buffer buf;
    assert(!init_buffer(&buf, 6));
    CU_ASSERT_EQUAL(buffer_find(&buf, "ab"), -1);
    memcpy(buffer_start(&buf), test, 6);
    buffer_shift(&buf, strlen(test));
    CU_ASSERT_EQUAL(buffer_find(&buf, test), 0);
    CU_ASSERT_EQUAL(buffer_find(&buf, "bb"), 2);
    CU_ASSERT_EQUAL(buffer_find(&buf, "cc"), 4);
    CU_ASSERT_EQUAL(buffer_find(&buf, "bcc"), 3);
    CU_ASSERT_EQUAL(buffer_find(&buf, "ccc"), -1);
    CU_ASSERT_EQUAL(buffer_find(&buf, "d"), -1);
    CU_ASSERT_EQUAL(buffer_space(&buf), 0);
    cut = buffer_cut(&buf, 2);
    CU_ASSERT_STRING_EQUAL(cut, "aa");
    free(cut);
    cut = buffer_cut(&buf, 2);
    CU_ASSERT_STRING_EQUAL(cut, "bb");
    free(cut);
    CU_ASSERT_EQUAL(buf.pos, 2);
    CU_ASSERT_EQUAL(buffer_space(&buf), 4);
    memcpy(buffer_start(&buf), test, 4);
    buffer_shift(&buf, 4);
    CU_ASSERT(!is_buffer_empty(&buf));
    cut = buffer_cut(&buf, 2);
    CU_ASSERT_STRING_EQUAL(cut, "cc");
    free(cut);
    cut = buffer_cut(&buf, 2);
    CU_ASSERT_STRING_EQUAL(cut, "aa");
    free(cut);
    cut = buffer_cut(&buf, 2);
    CU_ASSERT_STRING_EQUAL(cut, "bb");
    free(cut);
    CU_ASSERT(is_buffer_empty(&buf));
    destroy_buffer(&buf);
}

void test_fail(void)
{
    char s[1024];
    REDIRECT_OUTPUTS;
    struct buffer buf;
    CU_ASSERT_EQUAL(init_buffer(&buf, SSIZE_MAX), -1);
    READ_ERR(s);
    CU_ASSERT(strstr(s, "alloc") != NULL);
    RESTORE_OUTPUTS;
}

int main()
{
    int r = EXIT_SUCCESS;
    if (CU_initialize_registry() != CUE_SUCCESS)
        goto exit;
    CU_pSuite suite = CU_add_suite("buffer", NULL, NULL);
    if (!suite)
        goto cleanup;
    if (!CU_add_test(suite, "buffer tests", test_buffer))
        goto cleanup;
    if (!CU_add_test(suite, "out of memory test", test_fail))
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

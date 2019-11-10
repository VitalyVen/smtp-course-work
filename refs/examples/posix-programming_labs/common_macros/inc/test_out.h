#ifndef __TEST_OUT_H__
#define __TEST_OUT_H__

#include <assert.h>
#include <stdio.h>
#include <unistd.h>

#define REDIRECT_OUTPUTS \
    FILE *readout, *readerr; \
    int errpipe[2], outpipe[2]; \
    assert(!pipe(errpipe)); \
    assert(!pipe(outpipe)); \
    fflush(stdout); \
    fflush(stderr); \
    int old_out = dup(1); \
    int old_err = dup(2); \
    assert(dup2(outpipe[1], 1) != -1); \
    assert(dup2(errpipe[1], 2) != -1); \
    assert(readout = fdopen(outpipe[0], "r")); \
    assert(readerr = fdopen(errpipe[0], "r"))

#define RESTORE_OUTPUTS \
    fflush(stdout); \
    fflush(stderr); \
    fclose(readerr); \
    fclose(readout); \
    assert(dup2(old_out, 1) != -1); \
    assert(dup2(old_err, 2) != -1);

#define READ_OUT(s) do { \
        fflush(stdout); \
        fgets(s, sizeof(s), readout); \
    } while (0)

#define READ_ERR(s) do { \
        fflush(stderr); \
        fgets(s, sizeof(s), readerr); \
    } while (strstr(s, "[DEBUG]") != NULL)

#define READ_DEBUG(s) do { \
        fflush(stderr); \
        fgets(s, sizeof(s), readerr); \
    } while (0)

#endif

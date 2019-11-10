#ifndef __ERRORS_H__
#define __ERRORS_H__

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#define warning(format, ...) do { \
    fprintf(stderr, "[WARNING] "format"\n", ##__VA_ARGS__); \
    fflush(stderr); \
} while (0)

#ifdef DEBUG
#define debug(format, ...) do {\
    fprintf(stderr, "[DEBUG][%d] "format"\n", \
            (int) getpid(), ##__VA_ARGS__); \
    fflush(stderr); \
} while (0)
#else
#define debug(format, ...) ((void) 0)
#endif

#define error(format, ...) do { \
    fprintf(stderr, "[ERROR][%d][%s:%s:%d] "format"\n", \
            (int) getpid(), __FILE__, __func__, __LINE__, ##__VA_ARGS__); \
    fflush(stderr); \
} while (0)

#define errnomsg(format, ...) \
    error(format": %s", ##__VA_ARGS__, strerror(errno))

#define errmsg(code, format, ...) \
    error(format": %s", ##__VA_ARGS__, strerror(code))

#define fatal(result, format, ...) do { \
    int code = (result); \
    if (code) { \
        errmsg(code, format,  ##__VA_ARGS__); \
        exit(EXIT_FAILURE); \
    } \
} while (0)


// Note: there is no "do ... while(0)" here so
// we can use "break" or "continue" as arguments.
#define CHECK_EINTR(eintr_op, op, msg, ...) { \
    if (errno == EINTR) { \
        debug(msg": %s", ##__VA_ARGS__, strerror(errno)); \
        eintr_op; \
    } else  { \
        errnomsg(msg, ##__VA_ARGS__); \
        op; \
    } \
}

// Note: there is no "do ... while(0)" here so
// we can use "break" or "continue" as arguments.
#define CHECK_EINTREAGAIN(eintr_op, eagain_op, op, msg, ...) { \
    if (errno == EINTR) { \
        debug(msg": %s", ##__VA_ARGS__, strerror(errno)); \
        eintr_op; \
    } else if (errno == EAGAIN || errno == EWOULDBLOCK) { \
        debug(msg": %s", ##__VA_ARGS__, strerror(errno)); \
        eagain_op; \
    } else  { \
        errnomsg(msg, ##__VA_ARGS__); \
        op; \
    } \
}

#endif

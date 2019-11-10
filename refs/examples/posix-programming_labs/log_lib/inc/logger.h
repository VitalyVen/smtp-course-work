#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <sys/types.h>

#include "redir.h"

#pragma pack(0)

enum {
    R_OUT = 0,
    R_ERR = 1,
    R_FIRST= R_OUT,
    R_LAST = R_ERR,
    R_COUNT = R_LAST - R_FIRST + 1
};

struct logger {
        struct redir r[R_COUNT];
};

int logger_init(struct logger *logger, process_t out, process_t err);

// On success, 1 is returned in parent process and 0 in child process.
// On error, -1 is returned.
int logger_start(struct logger *logger);

int logger_polled_start(struct logger *logger);

void logger_stop(struct logger *logger);

int set_sig_handlers(struct logger *logger);
void clear_sig_handlers();

#endif


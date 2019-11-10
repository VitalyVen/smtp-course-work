#include <signal.h>

#include "logger.h"

int logger_init(struct logger *logger, process_t out, process_t err)
{
    if (init_redir(&logger->r[R_OUT], out, stdout))
        return -1;
    if (init_redir(&logger->r[R_ERR], err, stderr))
        return -1;
    return 0;
}

int logger_start(struct logger *logger)
{
    for (int i = R_FIRST; i <= R_LAST; i++) {
        pid_t pid = fork_redir(&logger->r[i]);
        if (pid <= 0) // Error detected or child process is running
            return pid;
    }
    return 1;
}

int logger_polled_start(struct logger *logger)
{
    pid_t pid;
    pid = fork_redirs(logger->r, 2);
    if (pid <= 0) // Error detected or child process is running
        return pid;
    return 1;
}

void logger_stop(struct logger *logger)
{
    restore_redir(&logger->r[R_OUT]);
    restore_redir(&logger->r[R_ERR]);
    waitpid_redir(&logger->r[R_OUT]);
    waitpid_redir(&logger->r[R_ERR]);
}

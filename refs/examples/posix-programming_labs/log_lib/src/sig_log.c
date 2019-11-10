#include <signal.h>

#include "logger.h"

static struct logger *siglogger;

void sigpipe_handler(int signal)
{
    if (!siglogger)
        return;
    for (int i = R_FIRST; i <= R_LAST;i++)
        restore_broken_redir(&siglogger->r[i]);
}

int set_sig_handlers(struct logger *logger)
{
    siglogger = logger;
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_NOCLDWAIT;
    sa.sa_handler = SIG_IGN;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction()");
        return -1;
    }
    sa.sa_flags = 0;
    sa.sa_handler = sigpipe_handler;
    if (sigaction(SIGPIPE, &sa, NULL) == -1) {
        perror("sigaction()");
        return -1;
    }
    return 0;
}

void clear_sig_handlers()
{
    siglogger = NULL;
}

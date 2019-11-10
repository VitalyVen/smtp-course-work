#include <signal.h>

#include "errors.h"
#include "sig_handler.h"

bool running = true;

static void term_handler(int signal)
{
    running = false;
}

int set_termhandler(void)
{
    struct sigaction sa = {
        .sa_flags = 0,
        .sa_handler = term_handler
    };
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        errnomsg("sigaction(SIGTERM)");
        return -1;
    }
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        errnomsg("sigaction(SIGINT)");
        return -1;
    }
    return 0;
}

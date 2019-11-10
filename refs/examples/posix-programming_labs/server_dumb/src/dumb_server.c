#include <stdbool.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

#include "cmds.h"
#include "cnct_io.h"
#include "dumb_server.h"
#include "errors.h"
#include "fd_utils.h"
#include "sig_handler.h"

void run_server(int binded_socket, struct vdb *db)
{
    if (set_termhandler())
        return;
    while (running) {
        debug("Accepting...");
        int client_socket = accept(binded_socket, NULL, 0);
        if (client_socket == -1)
            CHECK_EINTR(continue, return, "accept()");
        debug("New connection accepted.");
        struct cnct cnct;
        if (init_fd_cnct(&cnct, client_socket))
            return;
        while (running && process_cnct(&cnct, db) >= 0)
            ;
        debug("Closing connection.");
        safe_close(client_socket);
        destroy_cnct(&cnct);
    }
}

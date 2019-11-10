#include <stdio.h>
#include <stdlib.h>

#include "dumb_server.h"
#include "fd_utils.h"
#include "db_simple.h"
#include "errors.h"

void usage(void)
{
    printf("Usage: ./server_dumb <socket_name>\n");
    printf("Example: ./server_dumb /tmp/db_socket\n");
    exit(1);
}

int main(int argc, char **argv)
{
    if (argc != 2)
        usage();
    char *socket_name = argv[1];
    int binded_socket = create_binded_socket(socket_name);
    if (binded_socket == -1) {
        error("There is a failure when trying to use a socket \"%s\".",
              socket_name);
        return -1;
    }
    struct simple_db simple_db;
    struct vdb *db = init_simple_db(&simple_db, NULL);
    printf("dumb server uses socket \"%s\" (pid = %d)\n",
           socket_name, getpid());
    run_server(binded_socket, db);
    db->destroy(db);
    return 0;
}

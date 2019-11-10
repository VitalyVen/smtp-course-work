#include <stdio.h>
#include <stdlib.h>

#include "errors.h"
#include "dblib.h"
#include "poll_server.h"

void usage(void)
{
    printf("Usage: ./server <path/to/db_lib> <socket_name> <max_connections>\n");
    printf("Example: ./server ../db_libs/libdbsimple.so /tmp/db_socket 128\n");
}

int main(int argc, char **argv)
{
    setlinebuf(stdout);
    if (argc != 4) {
        usage();
        return 1;
    }
    int max_connections = atoi(argv[3]);
    if (max_connections <= 0) {
        error("Second parameter should be greater than zero.");
        usage();
        return 1;
    }
    char *socket_name = argv[2];
    if (load_db_lib(argv[1]))
        exit(EXIT_FAILURE);
    printf("db lib: %s, socket: \"%s\", max. clients: %d\n",
           db_name(), socket_name, max_connections);
    struct vdb *db = create_db(async_response);
    assert(db);
    struct poll_server server;
    init_server(&server, max_connections, db);
    if (bind_server(&server, socket_name))
        usage();
    else {
        printf("poll()-based server is running (pid = %d)\n", getpid());
        run_server(&server);
    }
    destroy_server(&server);
    db->free(db);
    return 0;
}


#include <stdio.h>
#include <stdlib.h>

#include "errors.h"
#include "db_lock.h"
#include "thread_server.h"

void usage(void)
{
    printf("Usage: ./server <socket_name>\n");
    printf("Example: ./server /tmp/db_socket\n");
}

int main(int argc, char **argv)
{
    setlinebuf(stdout);
    if (argc != 2) {
        usage();
        return 1;
    }
    char *socket_name = argv[1];
    struct lock_db lock_db;
    struct vdb *db = init_lock_db(&lock_db, NULL);
    struct thread_server server;
    init_server(&server, db);
    if (bind_server(&server, socket_name))
        usage();
    else {
        printf("pthread-based server is running using socket \"%s\" (pid = %d)\n",
                socket_name, getpid());
        run_server(&server);
    }
    destroy_server(&server);
    db->destroy(db);
}

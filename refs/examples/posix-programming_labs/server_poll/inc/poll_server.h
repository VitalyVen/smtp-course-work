#ifndef __POLL_SERVER_H__
#define __POLL_SERVER_H__

#include <poll.h>

#include "wrbuffer.h"
#include "db_virtual.h"
#include "cnct.h"

#define CNCT(server, i) (&(server)->cncts[i].cnct)
#define FD(server, i) ((server)->fds[i].fd)

struct poll_server;

struct poll_cnct {
    struct cnct cnct;
    struct poll_server *server;
};

struct answer {
    int nfd;
    char *value;
};

// Polling server state structure.
struct poll_server {
    struct pollfd *bindedfd;  // Binded server socket.
    struct pollfd *controlfd;
    int control_input;
    int max_cnct;       // Max. number of cnctections.
    struct pollfd *fds; // This array should contain 'max_cnct + 1' items.
    struct poll_cnct *cncts;
    struct wrbuffer *wrbufs;
    struct vdb *db;
};

int init_server(struct poll_server *server, int max_cnct,
                 struct vdb *db);

void destroy_server(struct poll_server *server);

// Find free fd slot and occupy it.
// Return -1 if there is no free slots.
int add_fd(struct poll_server *server, int socket);

// Accept a new client cnctection and change server state accordingly.
// Return new connection slot number
int accept_connection(struct poll_server *server);

// Close a client cnctection and change server state accordingly.
void remove_connection(struct poll_server *server, int nfd);

//
int bind_server(struct poll_server *server, char *socket_name);

//
void process_fd(struct poll_server *server, int nfd);

void process_control(struct poll_server *server);

void process_client(struct poll_server *server, int nfd);

void run_server(struct poll_server *server);

void poll_fds(struct poll_server *server);

void async_response(char *key, char *value, void *arg);

void send_answer(struct poll_cnct *cnct, char *value);

#endif

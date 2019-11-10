#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include "buffer.h"
#include "errors.h"
#include "fd_utils.h"
#include "telnet_io.h"
#include "telnet_msg.h"

int run(char *socket_name)
{
    int r = -1;
    int sock = socket(PF_LOCAL, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Error: socket()");
        goto exit;
    }
    struct sockaddr_un address = {.sun_family = AF_LOCAL};
    strcpy(address.sun_path, socket_name);
    if (connect(sock, (struct sockaddr *) &address, sizeof(address))) {
        perror("Error: connect()");
        goto exit;
    }
    printf("Connected to \"%s\", enter \"\\q\" to quit.\n", socket_name);
    struct buffer answer;
    if (init_buffer(&answer, 1024 *64))
        goto close;
    while (1) {
        char req[1024];
        fgets(req, sizeof(req) - TELNET_SUFFIX_LEN, stdin);
        ssize_t len = strlen(req);
        if (len > 0 && req[len - 1] == '\n')
            req[len - 1] ='\0';
        if (!strcmp(req, "\\q"))
            break;
        strcat(req, TELNET_SUFFIX);
        if (send_all(sock, req, strlen(req)) == -1)
            goto free_buf;
        ssize_t nrecv = recv_until(sock, &answer, TELNET_SUFFIX);
        if (nrecv == 0)
            fprintf(stderr, "Invalid response.\n");
        if (nrecv < 0)
            goto free_buf;
        char *ans = buffer_cut(&answer, len);
        if (!ans)
            goto free_buf;
        ans[len - 1 - TELNET_SUFFIX_LEN] ='\0';
        printf("%s\n", ans);
        free(ans);
    }
    r = 0;
free_buf:
    destroy_buffer(&answer);
close:
    safe_close(sock);
exit:
    return r;
}

void usage(void)
{
    printf("Usage: ./client <socket_name>\n");
    printf("Example: ./client /tmp/db_socket\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    if (argc != 2)
        usage();
    if (run(argv[1]))
        exit(EXIT_FAILURE);
    else
        exit(EXIT_SUCCESS);
}

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "cmds.h"
#include "cnct_io.h"
#include "errors.h"
#include "fd_utils.h"
#include "telnet_io.h"
#include "telnet_msg.h"

int process_request(struct cnct *cnct, char *request, struct vdb *db)
{
    int r = -1;
    int to_send;
    char *result = process_cmd(request, db, &cnct->async_arg,
                               &to_send);
    if (!to_send) {
        r = 0;
        goto free;
    }
    if (result)
        if (send_all(cnct_fd(cnct), result, strlen(result)) == -1)
            goto free;
    if (send_all(cnct_fd(cnct), TELNET_SUFFIX, TELNET_SUFFIX_LEN) == -1)
            goto free;
    r = 0;
free:
    free(result);
    return r;
}

int process_cnct(struct cnct *cnct, struct vdb *db)
{
    int r = -1;
    struct buffer *buf = &cnct->buf;
    size_t spc = buffer_space(buf);
    if (!spc) {
        error("Receive buffer is full.");
        return -1;
    }
    ssize_t len = safe_recv(cnct_fd(cnct), buffer_start(buf), spc);
    if (len < 0)
        return -1;
    if (len == 0)
        return 0;
    buffer_shift(buf, len);
    ssize_t pos = buffer_find(buf, TELNET_SUFFIX);
    if (pos == -1)
        return 0;
    char *request_line = buffer_cut(buf, pos + TELNET_SUFFIX_LEN);
    if (!request_line)
        return 0; // out of memory
    if (!strip_telnet_cmd(request_line))
        if (process_request(cnct, request_line, db) != -1)
            r = 1;
    free(request_line);
    return r;
}

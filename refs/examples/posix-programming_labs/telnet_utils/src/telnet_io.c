#include <assert.h>
#include <string.h>
#include <sys/types.h>

#include "buffer.h"
#include "fd_utils.h"
#include "telnet_io.h"
#include "telnet_msg.h"

ssize_t recv_until(int sockfd, struct buffer *buf, char *sep)
{
    while (1) {
        ssize_t pos = buffer_find(buf, sep);
        if (pos != -1) // Separator is found.
            return pos + strlen(sep);
        size_t spc = buffer_space(buf);
        if (!spc)
            return 0; // Buffer is full.
        ssize_t len = safe_recv(sockfd, buffer_start(buf), spc);
        if (len < 0)
            return -1;
        buffer_shift(buf, len);
    }
}

ssize_t send_wsuffix(int sockfd, char *line, char *suffix)
{
    ssize_t nsent1 = 0;
    if (line) {
        nsent1 = send_all(sockfd, line, strlen(line));
        if (nsent1 == -1)
            return -1;
    }
    ssize_t nsent2 = send_all(sockfd, suffix, strlen(suffix));
    if (nsent2 == -1)
        return -1;
    return nsent1 + nsent2;
}

int strip_telnet_cmd(char *cmd)
{
    assert(cmd);
    ssize_t len = strlen(cmd);
    if (len < TELNET_SUFFIX_LEN)
        return -1;
    if (strcmp(cmd + len - TELNET_SUFFIX_LEN, TELNET_SUFFIX))
        return -1;
    cmd[len - TELNET_SUFFIX_LEN] = '\0';
    return 0;
}

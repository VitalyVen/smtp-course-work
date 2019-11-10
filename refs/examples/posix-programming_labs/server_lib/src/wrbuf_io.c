#include "fd_utils.h"
#include "wrbuf_io.h"

ssize_t send_wrbuffer(struct wrbuffer *buf, int socket)
{
    ssize_t sent_total = 0;
    while (!is_wrbuffer_empty(buf)) {
        ssize_t to_send = wrbuffer_remained(buf);
        ssize_t nsent = safe_send(socket, wrbuffer_start(buf), to_send);
        if (nsent < 0)
            return -1;
        if (nsent == 0)
            break;
        sent_total += nsent;
        wrbuffer_shift(buf, nsent);
        if (nsent != to_send)
            break; // partial send detected, lets quit
    }
    return sent_total;
}

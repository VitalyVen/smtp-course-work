#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "cmds.h"
#include "errors.h"
#include "telnet_msg.h"

char *process_cmd(char *request, struct vdb *db,
            struct async_arg *arg, int *to_send)
{
    assert(request);
    debug("request: [%s]", request);
    if (to_send)
        *to_send = 1;
    switch (request[0]) {
    case '=': {
        // Command: "=key value", value could be empty.
        char *sep = strchr(request, ' ');
        if (sep) {
            *sep = '\0';
            char *key = request + 1;
            char *value = sep + 1;
            db->set(db, key, value);
        }
    }
    break;
    case '?': { // Command: "?key"
        char *key = request + 1;
        if (!db->async_result) {
            assert(db->get);
            char *value = db->get(db, key);
            if (value)
                return value;
        }
        else {
            assert(db->geta);
            if (arg && arg->before_async_get)
                arg->before_async_get(arg);
            db->geta(db, key, arg);
            if (to_send)
                *to_send = 0;
        }
    }
    break;
    case '-': {  // Command: "-key"
        char *key = request + 1;
        db->set(db, key, NULL);
    }
    break;
    }
    return NULL;
}

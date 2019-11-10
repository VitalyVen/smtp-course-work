#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "db_simple.h"
#include "cmds.h"

void run(void)
{
    struct simple_db simple_db;
    struct vdb *db = init_simple_db(&simple_db, NULL);
    char s[200];
    while (1) {
        fgets(s, sizeof(s), stdin);
        s[strlen(s) - 1] = 0;
        if (!strcmp(s, "\\q"))
            break;
        int to_send;
        char *answer = process_cmd(s, db, NULL, &to_send);
        assert(to_send);
        if (answer) {
            printf("%s\n", answer);
            free(answer);
        }
    }
    db->destroy(db);
}

int main()
{
    printf("Available commands:\nGet value: ?key\nSet value: =key value\nQuit: \\q\n\n");
    run();
    return 0;
}

#ifndef __DBLIB_H__
#define __DBLIB_H__

#include <dlfcn.h>
#include <stdio.h>

#include "db_virtual.h"
#include "errors.h"

typedef struct vdb *(*create_db_t)(db_async_result_t async_result);

void *db_lib;
create_db_t create_db;
const char *(*db_name)(void);

static inline
void close_db_lib(void)
{
    if (db_lib) {
        if (dlclose(db_lib))
            errnomsg("%s", dlerror());
        db_lib = NULL;
    }
    create_db = NULL;
    db_name = NULL;
}

static inline
int load_db_lib(char *lib_file)
{
    void *db_lib = dlopen(lib_file, RTLD_LAZY);
    if (!db_lib) {
        error("%s", dlerror());
        return -1;
    }
    db_name = dlsym(db_lib, "name");
    if (!db_name)
        goto dl_error;
    create_db = dlsym(db_lib, "create_db");
    if (!create_db)
        goto dl_error;
    return 0;
dl_error:
    error("%s", dlerror());
    close_db_lib();
    return -1;
}

#endif

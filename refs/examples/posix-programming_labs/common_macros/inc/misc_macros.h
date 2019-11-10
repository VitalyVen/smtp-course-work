#ifndef __MISC_MACROS_H__
#define __MISC_MACROS_H__

#include <stddef.h>

#define lengthof(x) (sizeof(x)/sizeof(x[0]))

#define containerof(ptr, type, field_name) ( \
    (type *) ((char *) ptr - offsetof(type, field_name)))

#endif

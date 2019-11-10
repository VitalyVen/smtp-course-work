#include "string_hash.h"

#define MAX_HASH 16

int string_hash(char *string)
{
    int result = 0;
    for (int n =0; string[n] != '\0' && n < MAX_HASH; n++)
        result += string[n] >> (n >> 1);
    return result & 0x0ff;
}

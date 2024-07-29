#ifndef __HASH_H__
#define __HASH_H__
#include <string.h>

unsigned int elfHash(char *string)
{
    unsigned int hash = 0;
    unsigned int x = 0;

    for (unsigned int i = 0; i < strlen(string); i++)
    {
        hash = (hash << 4) + string[i];
        if ((x = hash & 0xF0000000L) != 0)
        {
            hash ^= (x >> 24);
            hash &= ~x;
        }
    }
    return hash;
}

#endif

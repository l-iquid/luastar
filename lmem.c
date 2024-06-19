/*
 * Lua* memory utilities.
*/
#include <stdio.h>
#include <stdlib.h>
#include "In.h"



void* _LuaMem_Alloc(size_t siz, char* file, int line) {
    void* m = malloc(siz);
    if (m == NULL) {
        fprintf(stderr, "[Lua*-Memory]::%s:%d: Bad allocation.\n", file, line);
        exit(EXIT_FAILURE);
    }
    return m;
}

void* _LuaMem_Realloc(void* block, size_t siz, char* file, int line) {
    if (block == NULL) {
        fprintf(stderr, "[Lua*-Memory]::%s:%d: Bad reallocation; block is NULL.\n", file, line);
        exit(EXIT_FAILURE);
    }
    void* ptr = realloc(block, siz);
    if (ptr == NULL) {
        fprintf(stderr, "[Lua*-Memory]::%s:%d: Bad reallocation.\n", file, line);
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void* _LuaMem_Calloc(size_t amnts, size_t siz, char* file, int line) {
    char* m = LuaMem_Alloc(amnts*siz);
    for (int i = 0; i < amnts*siz; i++) {
        m[i] = 0;
    }
    return (void*)m;
}

void _LuaMem_Free(void* block, char* file, int line) {
    if (block == NULL) {
        fprintf(stderr, "[Lua*-Memory]::%s:%d: Unable to free; block is NULL.\n", file, line);
        return;
    }
    free(block);
}

/*
 * Contains utilities for the Lua* compiler (not memory related).
*/
#include "In.h"
#include <stdio.h>
#include <stdlib.h>

/* ptrvec */
PtrVec* LuaUtil_Init_PtrVec(void) {
    PtrVec* pv = LuaMem_Alloc(sizeof(PtrVec));
    pv->items = LuaMem_Alloc(sizeof(void*)*8);
    pv->cap = 8;
    pv->siz = 0;
    pv->tail = NULL;
    return pv;
}

void LuaUtil_PtrVec_Push(PtrVec* pv, void* i) {
    if (pv->siz + 1 > pv->cap) {
        pv->cap += 4;
        pv->items = LuaMem_Realloc(pv->items, sizeof(void*)*pv->cap);
    }   
    pv->items[pv->siz] = i;
    pv->siz++;
    pv->tail = pv->items[pv->siz-1];
}

void LuaUtil_PtrVec_Pop(PtrVec *pv) {
    pv->items[pv->siz-1] = NULL;
    pv->siz--;
    pv->tail = pv->items[pv->siz-1];
}

void LuaUtil_Free_PtrVec(PtrVec *pv) {
    LuaMem_Free(pv->items);
    LuaMem_Free(pv);
}

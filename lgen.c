/*
 * The Lua* backend.
 * Generates assembly code for the GNU Assembler (GAS).
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "In.h"



static char* add_quotes_to_string(char* str) {
    size_t ln = strlen(str);
    char* x = LuaMem_Zeroalloc(ln+3);
    sprintf(x, "\"%s\"\0", str);
    return x;
}
/* MAKES NEW STRING WITH HEAP ALLOCATION */
static char* remove_quotes_from_string(char* str) {
    size_t ln = strlen(str);
    char* x = LuaMem_Zeroalloc(ln);
    for (int i = 1; i < ln-1; i++)
        x[i-1] = str[i];
    return x;
}

static StringLiteral* retrieve_string_literal(char* lbl, GenState* gs) {
    for (int i = 0; i < gs->glb.string_literals->siz; i++) {
        StringLiteral* sl = gs->glb.string_literals->items[i];
        if (strcmp(sl->lbl, lbl) == 0)
            return sl;
    }
    return NULL;
}


/* assembly funcs */
// functions that handle directly putting down assembly

// NOTE: _strsiz must include null-terminator
#define cpystrto_asm(_x, _xsiz, _strsiz, _str)    {\
    _x[_xsiz] = LuaMem_Zeroalloc(_strsiz);        \
    for (int i = 0; i < _strsiz; i++)             \
        x[_xsiz][i] = _str[i];}

static char** logic_unit_start(void) {
    char** x = LuaMem_Alloc(sizeof(void*)*3);
    cpystrto_asm(x, 0, 6, ".text");
    cpystrto_asm(x, 1, 15, ".global _start");
    cpystrto_asm(x, 2, 8, ".start:");
    return x;
}
static char** jmp_exit(int EXIT_CODE) {
    char** x = LuaMem_Alloc(sizeof(void*)*2);
    char buf[20];
    sprintf(buf, "mov $%d, %rdi", EXIT_CODE);
    cpystrto_asm(x, 0, 20, buf);
    cpystrto_asm(x, 1, 9, "jmp EXIT");
    return x;
}
static char** exit_asm(void) {
    char** x = LuaMem_Alloc(sizeof(void*)*3);
    cpystrto_asm(x, 0, 6, "EXIT:");
    cpystrto_asm(x, 1, 14, "mov $60, %rax");
    cpystrto_asm(x, 2, 8, "syscall");
    return x;
}
static char** syswrite_fixed_siz(int STDBUFFER, char* LABEL_NAME, int LABEL_SIZ) {
    char** x = LuaMem_Alloc(sizeof(void*)*5);
    cpystrto_asm(x, 0, 20, "mov $1, %rax");
    char stdbufuh[20];
    char lblnmbufuh[20];
    char lblsizbufuh[20];
    sprintf(stdbufuh, "mov $%d, %rdi", STDBUFFER);
    sprintf(lblnmbufuh, "mov %s, %rsi", LABEL_NAME);
    sprintf(lblsizbufuh, "mov $%d, %rdx", LABEL_SIZ);
    cpystrto_asm(x, 1, 20, stdbufuh);
    cpystrto_asm(x, 2, 20, lblnmbufuh);
    cpystrto_asm(x, 3, 20, lblsizbufuh);
    cpystrto_asm(x, 4, 8, "syscall");
    return x;
}
static int strlit_counter = 0;
static char** string_literal_asm(char* STRLIT) {
    char** x = LuaMem_Alloc(sizeof(void*)*2);
    char slbf[20];
    char ascizbf[20+strlen(STRLIT)];
    sprintf(slbf, "SL%d:", strlit_counter);
    sprintf(ascizbf, ".asciz %s", STRLIT);
    cpystrto_asm(x, 0, 7, slbf);
    cpystrto_asm(x, 1, 8+strlen(STRLIT), ascizbf);
    strlit_counter++;
    return x;
}

#define push_func(fn, args, sz, asm_fn, _gs)   {\
    char** x = fn args ;                        \
    _gs->last_asm_fn_call = asm_fn;             \
    for (int i = 0; i < sz; i++) {              \
        LuaUtil_PtrVec_Push(go, x[i]);          \
    }                                           \
    LuaMem_Free(x);}

/* main */

// rodata
static void create_string_literal(PtrVec* go, GenState* gs, Node* nd) {
    char* quote_str = add_quotes_to_string(nd->v);
    push_func(string_literal_asm, (quote_str), 2, AFNC_NULL, gs);
    char* lbl = LuaMem_Zeroalloc(9);
    sprintf(lbl, "\"SL%d\0", strlit_counter-1);

    StringLiteral* sl = LuaMem_Alloc(sizeof(StringLiteral));
    sl->lbl = LuaMem_Zeroalloc(strlen(lbl));
    for (int i = 1; i < strlen(lbl); i++)
        sl->lbl[i-1] = lbl[i];
    sl->v = strdup(quote_str);
    LuaUtil_PtrVec_Push(gs->glb.string_literals, sl);

    LuaMem_Free(nd->v);
    LuaMem_Free(quote_str);
    nd->v = lbl;
}
static void evaluate_statement_rodata(PtrVec* go, GenState* gs, Node* nd) {
    switch (nd->k) {
        case ND_CALL_STATEMENT:
            for (int i = 0; i < nd->next.s; i++) {
                Node* nxt = nd->next.r[i];
                switch (nxt->k) {
                    case ND_STRING_LITERAL:
                        // string literal
                        create_string_literal(go, gs, nxt);
                        break;
                }
            }
            break;
    }
}
static void loopthrough_clause_rodata(PtrVec* go, GenState* gs, Node* nd) {
    switch (nd->k) {
        case ND_NULL:
            /* root */
            for (int i = 0; i < nd->next.s; i++)
                evaluate_statement_rodata(go, gs, nd->next.r[i]);
            break;
    }
}
static inline void rodata_stage(PtrVec* go, GenState* gs, ParseOut* po) {
    loopthrough_clause_rodata(go, gs, po->root);
}

/*********/
/* logic */
static inline void call_statement_logic(PtrVec* go, GenState* gs, Node* nd) {
  
    for (int i = 0; i < nd->next.s; i++) {
        Node* nxt = nd->next.r[i];

        switch (nxt->v[0]) {
            case '"':
                /* string literal */
                char* lbl = LuaMem_Zeroalloc(strlen(nxt->v));
                for (int j = 1; j < strlen(nxt->v); j++)
                    lbl[j-1] = nxt->v[j];
                StringLiteral* sl = retrieve_string_literal(lbl, gs);
                char* v = remove_quotes_from_string(sl->v);
                size_t sz = strlen(v);
                push_func(syswrite_fixed_siz, (1, lbl, sz), 5, AFNC_SYSWRITE_FIXED_SIZ, gs);
                LuaMem_Free(v);
                break;
        }
    }

}
static void evaluate_statement_logic(PtrVec* go, GenState* gs, Node* nd) {
    switch (nd->k) {
        case ND_CALL_STATEMENT:
            call_statement_logic(go, gs, nd);
            break;
    }
}
static void loopthrough_clause_logic(PtrVec* go, GenState* gs, Node* nd) {
    switch (nd->k) {
        case ND_NULL:
            /* root */
            for (int i = 0; i < nd->next.s; i++)
                evaluate_statement_logic(go, gs, nd->next.r[i]);
            break;
    }
}
static inline void logic_stage(PtrVec* go, GenState* gs, ParseOut* po) {
    loopthrough_clause_logic(go, gs, po->root);
}


/* API */
PtrVec* LuaGen_x86_64_Linux(ParseOut* po) {
    PtrVec* go = LuaUtil_Init_PtrVec();

    GenState gs = {
        .glb = {
            .string_literals = LuaUtil_Init_PtrVec(),
        },

        .last_asm_fn_call = AFNC_NULL,
    };

    //push_func(logic_unit_start, (), 3);

    //push_func(jmp_exit, (0), 2);
    //push_func(exit_asm, (), 3);
    
    /* #1: rodata */
    rodata_stage(go, &gs, po);

    /* #3: logic */
    push_func(logic_unit_start, (), 3, AFNC_NULL, (&gs));
    logic_stage(go, &gs, po);
    push_func(jmp_exit, (0), 2, AFNC_NULL, (&gs));

    /* finish */
    push_func(exit_asm, (), 3, AFNC_NULL, (&gs));

    for (int i = 0; i < go->siz; i++)
        printf("%s\n", go->items[i]);

    /* cleanup */
    for (int i = 0; i < gs.glb.string_literals->siz; i++) {
        StringLiteral* sl = gs.glb.string_literals->items[i];
        LuaMem_Free(sl->lbl);
        LuaMem_Free(sl->v);
        LuaMem_Free(sl);
    }
    LuaUtil_Free_PtrVec(gs.glb.string_literals);
    return go;
}
void LuaGen_Free(PtrVec *go) {
    for (int i = 0; i < go->siz; i++)
        LuaMem_Free(go->items[i]);
    LuaUtil_Free_PtrVec(go);
}

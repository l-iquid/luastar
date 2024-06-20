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
    x[0] = '"';
    x[ln+1] = '"';
    for (int i = 0; i < ln; i++)
        x[i+1] = str[i];
    return x;
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
    char** x = LuaMem_Alloc(20*5);
    x[0] = "mov $1, %rax";
    sprintf(x[1], "mov $%d, %rdi", STDBUFFER);
    sprintf(x[2], "lea %s, %rsi", LABEL_NAME);
    sprintf(x[3], "mov $%d, %rdx", LABEL_SIZ);
    x[4] = "syscall";
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
    sprintf(lbl, "\"SL%d", strlit_counter-1);

    StringLiteral* sl = LuaMem_Alloc(sizeof(StringLiteral));
    sl->lbl = strdup(lbl);
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
    logic_stage(go, &gs, go);
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

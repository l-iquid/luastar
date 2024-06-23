/*
 * The Lua* backend.
 * Generates assembly code for the GNU Assembler (GAS).
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "In.h"

static void loopthrough_clause_logic(PtrVec* go, GenState* gs, Node* nd);
static void loopthrough_clause_rodata(PtrVec* go, GenState* gs, Node* nd);

static inline bool is_number(char* n) {
    return n[0] >= '0' && n[0] <= '9' ? true : false;
}

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

#define sprintf_buf_asm(_x, _xsiz, _bufsiz, ...)  \
{ char buf[_bufsiz];                              \
    sprintf(buf, __VA_ARGS__);                    \
    cpystrto_asm(_x, _xsiz, _bufsiz, buf);}


static char** logic_unit_start(void) {
    char** x = LuaMem_Alloc(sizeof(void*)*3);
    cpystrto_asm(x, 0, 6, ".text");
    cpystrto_asm(x, 1, 15, ".global _start");
    cpystrto_asm(x, 2, 8, "_start:");
    return x;
}
static char** jmp_exit(int EXIT_CODE) {
    char** x = LuaMem_Alloc(sizeof(void*)*2);
    sprintf_buf_asm(x, 0, 20, "mov $%d, %rdi", EXIT_CODE);
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
    sprintf_buf_asm(x, 1, 20, "mov $%d, %rdi", STDBUFFER);
    sprintf_buf_asm(x, 2, 20, "lea %s, %rsi", LABEL_NAME);
    sprintf_buf_asm(x, 3, 20, "mov $%d, %rdx", LABEL_SIZ);
    cpystrto_asm(x, 4, 8, "syscall");
    return x;
}
static int strlit_counter = 0;
static char** string_literal_asm(char* STRLIT) {
    char** x = LuaMem_Alloc(sizeof(void*)*2);
    sprintf_buf_asm(x, 0, 20, "SL%d:", strlit_counter);
    sprintf_buf_asm(x, 1, 20+strlen(STRLIT), ".asciz %s", STRLIT);
    strlit_counter++;
    return x;
}
static int cmp_counter = 0;
static char** binary_if_asm(char* frst, char* scnd, NDKind operator, int has_else) {
    char** x = LuaMem_Alloc(sizeof(void*)*6);
    if (is_number(frst)) {
        sprintf_buf_asm(x, 0, 20, "mov $%s, %rax", frst);
    } else {
        sprintf_buf_asm(x, 0, 20, "mov %s, %rax", frst);
    }
    if (is_number(scnd)) {
        sprintf_buf_asm(x, 1, 20, "mov $%s, %rbx", scnd);
    } else {
        sprintf_buf_asm(x, 1, 20, "mov %s, %rbx", scnd);
    }
    cpystrto_asm(x, 2, 20, "cmp %rbx, %rax");
    switch (operator) {
        case ND_GREATER_THAN:
            sprintf_buf_asm(x, 3, 20, "jg CMP%d", cmp_counter);
            break;
        case ND_GREATER_EQUAL:
            sprintf_buf_asm(x, 3, 20, "jge CMP%d", cmp_counter);
            break;
        case ND_LESS_THAN:
            sprintf_buf_asm(x, 3, 20, "jl CMP%d", cmp_counter);
            break;
        case ND_LESS_EQUAL:
            sprintf_buf_asm(x, 3, 20, "jle CMP%d", cmp_counter);
            break;
        case ND_EQUAL_EQUAL:
            sprintf_buf_asm(x, 3, 20, "je CMP%d", cmp_counter);
            break;
    }
    if (has_else) {
        sprintf_buf_asm(x, 4, 20, "jmp ELSE%d", cmp_counter);
    } else {
        cpystrto_asm(x, 4, 20, "; NO ELSE DETECTED");
    }
    sprintf_buf_asm(x, 5, 30, "CMP%dReturnAfter:", cmp_counter);

    cmp_counter++;
    return x;
}
static char** binary_if_label_asm(char* lbl) {
    char** x = LuaMem_Alloc(sizeof(void*)*1);
    sprintf_buf_asm(x, 0, 8, "%s:", lbl);
    return x;
}
static char** binary_if_return_asm(char* lbl) {
    char** x = LuaMem_Alloc(sizeof(void*)*1);
    sprintf_buf_asm(x, 0, 30, "jmp %sReturnAfter", lbl);
    return x;
}
static char** ret_asm(void) {
    char** x = LuaMem_Alloc(sizeof(void*)*1);
    cpystrto_asm(x, 0, 4, "ret");
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

/*
==================================
RODATA
==================================
*/
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
        case ND_IF_STATEMENT:
            loopthrough_clause_rodata(go, gs, nd);
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
        case ND_IF_STATEMENT:
            for (int i = 1; i < nd->next.s; i++)
                evaluate_statement_rodata(go, gs, nd->next.r[i]);
            break;
    }
}
static inline void rodata_stage(PtrVec* go, GenState* gs, ParseOut* po) {
    loopthrough_clause_rodata(go, gs, po->root);
}


/*
==================================
LOGIC
==================================
*/
static inline void print_call_logic(PtrVec* go, GenState* gs, Node* nd) {
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

static inline void call_statement_logic(PtrVec* go, GenState* gs, Node* nd) {
    if (strcmp(nd->v, "print") == 0 || strcmp(nd->v, "println") == 0) {
        print_call_logic(go, gs, nd);
    }

}

static inline void if_statement_logic(PtrVec* go, GenState* gs, Node* nd) {
    /* get if statement info */
    Node* condition = nd->next.r[0];
    Node* frst = condition;
    Node* scnd = NULL;
    NDKind operator = ND_NULL;

    Node* nxt = condition;
    for (;;) {
        if (scnd) break;
        switch (nxt->k) {
            case ND_GREATER_THAN:
            case ND_GREATER_EQUAL:
            case ND_LESS_THAN:
            case ND_LESS_EQUAL:
            case ND_EQUAL_EQUAL:
                operator = nxt->k;
                scnd = nxt->next.r[0];
                break;
        }

        if (nxt->next.s > 0) {
            nxt = nxt->next.r[0];
        } else {
            break;
        }
    }

    push_func(binary_if_asm, (frst->v, scnd->v, operator, 0), 6, AFNC_BINARY_IF, gs);
    AsmFutureScope* fs = LuaMem_Alloc(sizeof(AsmFutureScope));
    fs->lbl = LuaMem_Zeroalloc(8);
    fs->nd = nd;
    sprintf(fs->lbl, "CMP%d", cmp_counter-1);
    LuaUtil_PtrVec_Push(gs->glb.future_scopes.cmps, fs);
    //loopthrough_clause_logic(go, gs, nd);
}

/* evaluate statement */
static void evaluate_statement_logic(PtrVec* go, GenState* gs, Node* nd) {
    switch (nd->k) {
        case ND_CALL_STATEMENT:
            call_statement_logic(go, gs, nd);
            break;
        case ND_IF_STATEMENT:
            if_statement_logic(go, gs, nd);
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
        case ND_IF_STATEMENT:
            for (int i = 1; i < nd->next.s; i++)
                evaluate_statement_logic(go, gs, nd->next.r[i]);
            break;
    }
}

/* future scopes */
static inline void logic_make_future_scopes(PtrVec* go, GenState* gs) {
    
    /* if statements */
    for (int i = 0; i < gs->glb.future_scopes.cmps->siz; i++) {
        AsmFutureScope* fs = gs->glb.future_scopes.cmps->items[i];
        push_func(binary_if_label_asm, (fs->lbl), 1, AFNC_BINARY_IF_LBL, gs);
        loopthrough_clause_logic(go, gs, fs->nd);
        push_func(binary_if_return_asm, (fs->lbl), 1, AFNC_BINARY_IF_RET, gs);
        //push_func(ret_asm, (), 1, AFNC_RET, gs);
    }

}

// main
static inline void logic_stage(PtrVec* go, GenState* gs, ParseOut* po) {
    push_func(logic_unit_start, (), 3, AFNC_NULL, gs);
    loopthrough_clause_logic(go, gs, po->root);
    push_func(jmp_exit, (0), 2, AFNC_NULL, gs);
    logic_make_future_scopes(go, gs);
    push_func(exit_asm, (), 3, AFNC_NULL, gs);
}

/* API */
PtrVec* LuaGen_x86_64_Linux(ParseOut* po) {
    PtrVec* go = LuaUtil_Init_PtrVec();

    GenState gs = {
        .glb = {
            .string_literals = LuaUtil_Init_PtrVec(),
            .future_scopes = {
                .cmps = LuaUtil_Init_PtrVec(),
            },
        },

        .last_asm_fn_call = AFNC_NULL,
    };

    //push_func(logic_unit_start, (), 3);

    //push_func(jmp_exit, (0), 2);
    //push_func(exit_asm, (), 3);
    
    /* #1: rodata */
    rodata_stage(go, &gs, po);

    /* #3: logic */
    logic_stage(go, &gs, po);

    /* finish */
    for (int i = 0; i < go->siz; i++)
       printf("%s\n", go->items[i]);

    /* cleanup */
    for (int i = 0; i < gs.glb.string_literals->siz; i++) {
        StringLiteral* sl = gs.glb.string_literals->items[i];
        LuaMem_Free(sl->lbl);
        LuaMem_Free(sl->v);
        LuaMem_Free(sl);
    }
    for (int i = 0; i < gs.glb.future_scopes.cmps->siz; i++) {
        AsmFutureScope* fs = gs.glb.future_scopes.cmps->items[i];
        LuaMem_Free(fs->lbl);
    }
    LuaUtil_Free_PtrVec(gs.glb.string_literals);
    LuaUtil_Free_PtrVec(gs.glb.future_scopes.cmps);
    return go;
}
void LuaGen_Free(PtrVec *go) {
    for (int i = 0; i < go->siz; i++)
        LuaMem_Free(go->items[i]);
    LuaUtil_Free_PtrVec(go);
}

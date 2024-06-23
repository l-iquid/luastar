/*
 * The Lua* parser. Generates a AST (organized syntax tree) from lexer tokens.
*/
#include "In.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* globals that are function calls */
static const char* fglobals[] = {
    "print", "println"
};


/***************/
/* PARSE STATE */
/***************/
static inline void copy_ps_to_po(ParseState* ps, ParseOut* po) { 
    po->nds = LuaMem_Alloc(sizeof(void*)*ps->ndlst.c);
    po->siz = ps->ndlst.s;
    po->root = ps->ndlst.r;
    for (int i = 0; i < ps->ndlst.s; i++)
        po->nds[i] = ps->ndlst.nds[i];
}
static inline void free_ps(ParseState* ps) {
    for (int i = 0; i < ps->scope_archive->siz; i++) {
        LuaMem_Free(ps->scope_archive->items[i]);
    }
    LuaMem_Free(ps->ndlst.nds);
    LuaUtil_Free_PtrVec(ps->scopes);
    LuaUtil_Free_PtrVec(ps->scope_archive);
}
static Node* create_nd(NDKind kind, Token* tk) {
    Node* nd = LuaMem_Alloc(sizeof(Node));
    nd->k = kind;
    nd->v = strdup(tk->v);
    nd->m.line = tk->line;
    nd->m.column = tk->column;
    nd->prev = NULL;
    nd->next.r = LuaMem_Alloc(sizeof(void*)*4);
    nd->next.c = 4;
    nd->next.s = 0;
    return nd;
}
static void set_nd_next(Node* parent, Node* child) {
    if (parent->next.s + 1 > parent->next.c) {
        parent->next.c += 2;
        parent->next.r = LuaMem_Realloc(parent->next.r, sizeof(void*)*parent->next.c);
    }
    parent->next.r[parent->next.s] = child;
    parent->next.s++;
}
static void insert_nd_to_ps(Node* nd, ParseState* ps) {
    if (ps->ndlst.s + 1 > ps->ndlst.c) {
        ps->ndlst.c += 4;
        ps->ndlst.nds = LuaMem_Realloc(ps->ndlst.nds, sizeof(void*)*ps->ndlst.c);
    }
    ps->ndlst.nds[ps->ndlst.s] = nd;
    ps->ndlst.s++;
}
/* auto set ps->next to last node */
#define auto_next(_ps) _ps->next = _ps->ndlst.nds[_ps->ndlst.s-1]

//scope
static Scope* create_scope(SKind kind, Node* nd) {
    Scope* sp = LuaMem_Alloc(sizeof(Scope));
    sp->k = kind;
    sp->nd = nd;
    return sp;
}
#define insert_scope_to_ps(sp, _ps) LuaUtil_PtrVec_Push(_ps->scope_archive, sp); LuaUtil_PtrVec_Push(_ps->scopes, sp)
#define pop_scope(_ps) LuaUtil_PtrVec_Pop(_ps->scopes)
#define access_last_scope(_ps) ((Scope*)_ps->scopes->tail)

/********/
/* main */
/* token analyzers */

// created var is 'nd'
#define make_nd_routine(k,tk)       \
    Node* nd = create_nd(k, tk);    \
    insert_nd_to_ps(nd, ps)

static inline void identifier(ParseState* ps, Token* tk) {
    make_nd_routine(ND_IDENTIFIER, tk);
    set_nd_next(ps->next, nd);
    auto_next(ps);
}
static inline void literal(ParseState* ps, Token* tk) {
    NDKind k;
    switch (tk->k) {
        case TK_STRING:
            switch (access_last_scope(ps)->k) {
                case SCOPE_CALL:
                    /* println check */
                    if (strcmp(access_last_scope(ps)->nd->v, "println")==0) {
                        char* newstr = LuaMem_Zeroalloc(strlen(tk->v)+3);
                        sprintf(newstr, "%s\\n\0", tk->v);
                        LuaMem_Free(tk->v);
                        tk->v = newstr;
                    }
                    break;
            }
            k = ND_STRING_LITERAL;
            break;
        case TK_NUMERIC:
            k = ND_NUMERIC_LITERAL;
            break;
        case TK_BOOLEAN:
            k = ND_BOOLEAN_LITERAL;
            break;
    }
    make_nd_routine(k, tk);
    set_nd_next(ps->next, nd);
    auto_next(ps);
}
static inline void keyword(ParseState* ps, Token* tk) {
    if (strcmp(tk->v, "local") == 0) {
        
    }
    if (strcmp(tk->v, "if") == 0) {
       make_nd_routine(ND_IF_STATEMENT, tk);
       set_nd_next(ps->next, nd);
       auto_next(ps);
    }
    if (strcmp(tk->v, "else") == 0) {
        
    }
}
static inline void function_call(ParseState* ps, Token* tk) {
    make_nd_routine(ND_CALL_STATEMENT, tk);
    set_nd_next(ps->next, nd);
    auto_next(ps);
}
static inline void global(ParseState* ps, Token* tk) {
    for (int i = 0; i < sizeof(fglobals)/sizeof(fglobals[0]); i++) {
        if (strcmp(tk->v, fglobals[i]) == 0) {
            function_call(ps, tk);
            break;
        }
    }
    
    
}
static inline void open_paren(ParseState* ps, Token* tk) {
    switch (tk->prev->k) {
        case TK_OPEN_PAREN:
            /* not a statement ? */
            return;
    }

    switch (ps->next->k) {
        /* scope make */
        case ND_CALL_STATEMENT:
            Scope* sp = create_scope(SCOPE_CALL, ps->next);
            insert_scope_to_ps(sp, ps);
            break;
        case ND_IF_STATEMENT: {
            Scope* sp = create_scope(SCOPE_CLAUSE, ps->next);
            insert_scope_to_ps(sp, ps);
            break;}
    }
}
static inline void close_paren(ParseState* ps, Token* tk) {

    /* scope removal */
    switch (access_last_scope(ps)->k) {
        case SCOPE_CALL:
            pop_scope(ps);
            break;
        default:
            /* (a) vs a */

            break;
    }
}
static inline void close_squirly(ParseState* ps, Token* tk) {
    
    /* scope removal */
    switch (access_last_scope(ps)->k) {
        case SCOPE_CLAUSE:
            pop_scope(ps);
            ps->set_next_to_scope = true;
            break;
    }
}
static inline void comma(ParseState* ps, Token* tk) {
    switch(access_last_scope(ps)->k) {
        case SCOPE_CALL:
            ps->set_next_to_scope = true;
            break;
    } 
}
static inline void colon(ParseState* ps, Token* tk) {
    
}
static inline void binary_operator(ParseState* ps, Token* tk) {
    NDKind k;

    switch (tk->k) {
        case TK_GREATER_THAN:
            k = ND_GREATER_THAN;
            break;
        case TK_GREATER_EQUAL:
            k = ND_GREATER_EQUAL;
            break;
        case TK_LESS_THAN:
            k = ND_LESS_THAN;
            break;
        case TK_LESS_EQUAL:
            k = ND_LESS_EQUAL;
            break;
        case TK_EQUAL_EQUAL:
            k = ND_EQUAL_EQUAL;
            break;
    }

    make_nd_routine(k, tk);
    set_nd_next(ps->next, nd);
    auto_next(ps);
}

static inline void consume_tokens(ParseState* ps, LexerOut* lo) {
     
    for (int i = 0; i < lo->siz; i++) {
        Token* tk = lo->tks[i];
        
        switch (tk->k) {
            case TK_QUOTE:
                break;
            case TK_OPEN_SQUIRLY:
            case TK_SEMICOLON:
                ps->set_next_to_scope = true;
               break;
            case TK_COLON:
                colon(ps, tk);
               break;
            case TK_OPEN_PAREN:
                open_paren(ps, tk);
                break;
            case TK_CLOSE_PAREN:
                close_paren(ps, tk);
                break;
            case TK_COMMA:
                comma(ps, tk);
                break;
            case TK_CLOSE_SQUIRLY:
                close_squirly(ps, tk);
                break;

            case TK_GREATER_THAN:
            case TK_GREATER_EQUAL:
            case TK_LESS_THAN:
            case TK_LESS_EQUAL:
            case TK_EQUAL_EQUAL:
                binary_operator(ps, tk);
                break;
            
            case TK_GLOBAL:
                global(ps, tk);
                break;

            case TK_KEYWORD:
                keyword(ps, tk);
                break;

            case TK_STRING:
            case TK_NUMERIC:
            case TK_BOOLEAN:
                literal(ps, tk);
                break;

            default:
                identifier(ps, tk);
                break;
        }


        if (ps->set_next_to_scope) {
            ps->set_next_to_scope = false;
            ps->next = access_last_scope(ps)->nd;
        }
    }

}


/* API */
ParseOut* LuaParse_Generate_Tree(LexerOut* lo) {
    ParseOut* po = LuaMem_Alloc(sizeof(ParseOut));
    
    ParseState ps = {
        .scopes = LuaUtil_Init_PtrVec(),
        .scope_archive = LuaUtil_Init_PtrVec(),
        .next = NULL,
        .set_next_to_scope = false, 

        .ndlst = {
            .nds = LuaMem_Alloc(sizeof(void*)*8),
            .r = NULL,
            .s = 0,
            .c = 8,
        },

    };

    /* create root */
    Node* root = create_nd(ND_NULL, &(Token){.v=""});
    insert_nd_to_ps(root, &ps);
    ps.ndlst.r = ps.ndlst.nds[0];
    insert_scope_to_ps(create_scope(SCOPE_NULL, root), (&ps));
    ps.next = ps.ndlst.r;

    consume_tokens(&ps, lo);

    /* put ndlst into po */
    copy_ps_to_po(&ps, po);

    /* cleanup */
    free_ps(&ps);
    return po;
}
void LuaParse_Free_Tree(ParseOut *po) {
    for (int i = 0; i < po->siz; i++)
        LuaMem_Free(po->nds[i]);
    LuaMem_Free(po->nds);
    LuaMem_Free(po);
}
static inline void print_node(Node* nd, int layer) {
    for (int i = 0; i < layer; i++)
        printf("  ");
    printf(">> kind: %d. value: %s\n", nd->k, nd->v);
    for (int i = 0; i < nd->next.s; i++)
        print_node(nd->next.r[i], layer+1);
}
void LuaParse_Print_Tree(ParseOut* po) {
    printf("====AST====\n");
    print_node(po->root, 0);
    printf("===========\n");
}

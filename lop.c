/*
 * The Lua* bytecode middle-end.
 * Generates bytecodes for the back-end.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "In.h"



/* adds quotes around a string */
static char* stringify(char* v) {
    size_t ln = strlen(v);
    char* str = LuaMem_Zeroalloc(ln+3);
    str[0] = '"';
    str[ln+2] = '"';
    for (int i = 0; i < ln; i++)
        str[i+1] = v[i];
    return str;
}
/* if literal is string, add quotes around it */
static char* literal_could_be_string(NDKind k, char* v) {
    if (k == ND_STRING_LITERAL) return stringify(v);
    return strdup(v);
}

/* main */
#define node_lookahead(nd, _ops)          \
    for (int i = 0; i < nd->next.s; i++)  \
        evaluate_node(nd->next.r[i], _ops);


/* evaluates a literal node and puts it on opstate */
static void literal_node_evaluation(Node* nd, OPState* ops) {

    switch (nd->k) {
        /* evaluate literals */
        case ND_STRING_LITERAL:
           char* str = stringify(nd->v);
           LuaUtil_PtrVec_Push(ops->line.prms, str);
           break;

        case ND_NUMERIC_LITERAL:
            /* shitty arithmetic implementation TODO change */
            Node** arith_nodes = LuaMem_Alloc(sizeof(void*));
            size_t arith_nds_siz = 1;
            size_t arith_nds_cap = 1;
            arith_nodes[0] = nd;
            Node* cur_arith_nd = nd;
            while (cur_arith_nd->next.s > 0) {
                if (arith_nds_siz + 1 > arith_nds_cap) {
                    arith_nds_cap += 1;
                    arith_nodes = LuaMem_Realloc(arith_nodes, sizeof(void*)*arith_nds_cap);
                }
                arith_nodes[arith_nds_siz] = cur_arith_nd->next.r[0];
                arith_nds_siz++;
                cur_arith_nd = cur_arith_nd->next.r[0];
            }

            for (int i = 0; i < arith_nds_siz; i++)
                printf("%s\n", arith_nodes[i]->v);

            LuaMem_Free(arith_nodes);
            break;
        case ND_IDENTIFIER:
            /* potential expression */
            break;
        case ND_BOOLEAN_LITERAL:
            LuaUtil_PtrVec_Push(ops->line.prms, strdup(nd->v));
            break;
   }

}

/* call */
static inline void evaluate_call_parameter(Node* nd, OPState* ops) {
   
   switch (nd->k) {
        /* evaluate literals */
        case ND_STRING_LITERAL:
        case ND_NUMERIC_LITERAL:
        case ND_BOOLEAN_LITERAL:
        case ND_IDENTIFIER:
            literal_node_evaluation(nd, ops);
           break;
   }

}

static inline void call_statement(Node* nd, OPState* ops) {
    ops->line.cmd = CALL;
    LuaUtil_PtrVec_Push(ops->line.prms, strdup(nd->v)); /* function name */

    /* function parameters */
    for (int i = 0; i < nd->next.s; i++) {
        Node* nxt = nd->next.r[i];
        evaluate_call_parameter(nxt, ops);
    }
}


/* main evaluator */
static inline void evaluate_node(Node* nd, OPState* ops) {
    switch (nd->k) {
        case ND_NULL:
            /* root */
            node_lookahead(nd, ops);
            return;
        case ND_CALL_STATEMENT:
            call_statement(nd, ops);
            break;
    }

}

static inline void consume_tree(ParseOut* po, OPState* ops) {
    evaluate_node(po->root, ops);
}


/* API */
PtrVec* LuaOP_Generate(ParseOut* po) {
    PtrVec* opc = LuaUtil_Init_PtrVec();

    OPState ops = {
        .opls = LuaUtil_Init_PtrVec(),
        .line = {
            .cmd = NO_COMMAND,
            .prms = LuaUtil_Init_PtrVec(),
        },
    };

    consume_tree(po, &ops);

    /* cleanup */
    LuaUtil_Free_PtrVec(ops.opls);
    LuaUtil_Free_PtrVec(ops.line.prms);
    return opc;
}

static inline void free_opline(OPCodeLine* opl) {
    for (int i = 0; i < opl->prms->siz; i++) {
        LuaMem_Free(opl->prms->items[i]);
    }
    LuaUtil_Free_PtrVec(opl->prms);
    LuaMem_Free(opl);
}
void LuaOP_Free_Output(PtrVec *pv) {
    for (int i = 0; i < pv->siz; i++)
        free_opline(pv->items[i]);
    LuaUtil_Free_PtrVec(pv);
}

/*
 * The Lua* Lexer. Breaks down the source code into tokens.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "In.h"

/* configs */
#define LEXER_TKLST_INIT_CAP    8
#define LEXER_TKLST_REALLOC_CAP 8

static const char* lkeywords[] = {
    "local", "if", "else"
};
static const char* lglobals[] = {
    "print", "println"
};
static const char* ltypes[] = {
    "string",
    "i32", "u32"
};

#define is_in_ltab(tab,v)                                    \
    for (int i = 0; i < sizeof(tab)/sizeof(tab[0]); i++) {   \
        if (strcmp(v, tab[i]) == 0)                          \
            return 1;                                        \
    }                                                        \
    return 0;

static inline int is_keyword(char* v)
{is_in_ltab(lkeywords, v);}
static inline int is_lglobal(char* v)
{is_in_ltab(lglobals, v);}
static inline int is_ltype(char* v)
{is_in_ltab(ltypes, v);}
static inline int is_bool(char* v) {
    if (strcmp(v, "true") == 0 || strcmp(v, "false") == 0)
        return 1;
    return 0;
}

/************/
/* LEXSTATE */
/************/

/* Creates a blank new token. */
static Token* make_tk(TKKind kind, char* value, int line, int column) {
    Token* tk = LuaMem_Alloc(sizeof(Token));
    tk->k = kind;
    tk->v = strdup(value);
    tk->line = line;
    tk->column = column;
    tk->next = NULL;
    tk->prev = NULL;
    return tk;
}

static void push_tk_ls(LexState* ls, Token* tk) {
    if (ls->tklst.siz + 1 > ls->tklst.cap) {
        ls->tklst.cap += LEXER_TKLST_REALLOC_CAP;
        ls->tklst.tks = LuaMem_Realloc(ls->tklst.tks, sizeof(void*)*ls->tklst.cap); 
    }

    ls->tklst.tks[ls->tklst.siz] = tk;
    if (ls->tklst.siz > 0) {
        tk->prev = ls->tklst.tks[ls->tklst.siz-1];
        ls->tklst.tks[ls->tklst.siz-1]->next = tk;
    }
    ls->tklst.siz++;
}

#define create_token(k,v,l,c) push_tk_ls(ls, make_tk(k,v,l,c));

static inline void push_chr_to_tkbuf(LexState* ls, char CHR) {
    if (ls->tk_buf.siz + 2 > ls->tk_buf.cap) {
        ls->tk_buf.cap += 16;
        ls->tk_buf.buffer = LuaMem_Realloc(ls->tk_buf.buffer, ls->tk_buf.cap);
        for (int i = ls->tk_buf.cap-17; i < ls->tk_buf.cap; i++)
            ls->tk_buf.buffer[i] = 0;
    }
    ls->tk_buf.buffer[ls->tk_buf.siz] = CHR;
    ls->tk_buf.siz++;
}

#define push_str_to_tkbuf(ls, str)          \
    for (int i = 0; i < strlen(str); i++)   \
        push_chr_to_tkbuf(ls, str[i]);

static void clr_tkbuf(LexState* ls) {
    for (int i = 0; i < ls->tk_buf.siz; i++) {
        ls->tk_buf.buffer[i] = 0;
    }
    ls->tk_buf.siz = 0;
}

#define inesrt_tk_from_ls(k)                                    \
    if (ls->tk_buf.siz > 0) {                                   \
        create_token(k,ls->tk_buf.buffer,ls->line,ls->column);  \
    }                                                           \
    clr_tkbuf(ls);

#define tk_onechar(k,v)                               \
    if (ls->in_string) {                              \
        push_str_to_tkbuf(ls, v);                     \
        break;                                        \
    }                                                 \
    inesrt_tk_from_ls(TK_IDENTIFIER);                 \
    push_tk_ls(ls, make_tk(k,v,ls->line,ls->column));

#define tk_twochar(k1,v1,k2,v2) \
    switch (look[1]) {          \
        case '=': /* use 2 */   \
            tk_onechar(k2, v2); \
            *look++;            \
            break;              \
        default: /* use 1 */    \
            tk_onechar(k1, v1); \
            break;              \
    }

/********/
/* MAIN */
/********/

static inline void source_look(LexState* ls, char* in) {
    char* look = in;


    while (*look != '\0') {
       
        switch (*look) { 
            case '(': tk_onechar(TK_OPEN_PAREN, "("); break;
            case ')': tk_onechar(TK_CLOSE_PAREN, ")"); break;
            case ';': tk_onechar(TK_SEMICOLON, ";"); break;
            case ':': tk_onechar(TK_COLON, ":"); break;
            case ',': tk_onechar(TK_COMMA, ","); break;
            case '{': tk_onechar(TK_OPEN_SQUIRLY, "{"); break;
            case '}': tk_onechar(TK_CLOSE_SQUIRLY, "}"); break;
            case '>': tk_twochar(TK_GREATER_THAN, ">", TK_GREATER_EQUAL, ">="); break;
            case '<': tk_twochar(TK_LESS_THAN, "<", TK_LESS_EQUAL, "<="); break;
            case '=': tk_twochar(TK_EQUAL, "=", TK_EQUAL_EQUAL, "=="); break;
           
            case '\'': 
            case '"':
                if (!ls->in_string) {
                    /* string start */
                    tk_onechar(TK_QUOTE, "\"");
                    ls->in_string = true;
                    ls->string_clause = *look;
                } else {
                    if (ls->string_clause == *look) {
                        /* end string */
                        inesrt_tk_from_ls(TK_STRING);
                        ls->in_string = false;
                        tk_onechar(TK_QUOTE, "\"");
                    } else {
                        push_chr_to_tkbuf(ls, *look);
                    }
                }

                break;

            case 10: /* newline */
                ls->line++;
                ls->column = 0;
            case ' ':
                if (!ls->in_string) {
                    inesrt_tk_from_ls(TK_IDENTIFIER);
                } else {
                    push_chr_to_tkbuf(ls, *look);
                }
                break;

            default:
                push_chr_to_tkbuf(ls, *look);
                break;
        }

        *look++;
        ls->column++;
    }
    
    inesrt_tk_from_ls(TK_IDENTIFIER);

    /* make new types */
    for (int i = 0; i < ls->tklst.siz; i++) {
        Token* tk = ls->tklst.tks[i];
        
        switch (tk->k) {
            case TK_IDENTIFIER:
                if (tk->v[0] >= '0' && tk->v[0] <= '9') {
                    tk->k = TK_NUMERIC;
                    break;
                }
                if (is_keyword(tk->v)) {
                    tk->k = TK_KEYWORD;
                    break;
                }
                if (is_lglobal(tk->v)) {
                    tk->k = TK_GLOBAL;
                    break;
                }
                if (is_bool(tk->v)) {
                    tk->k = TK_BOOLEAN;
                    break;
                } 
                if (is_ltype(tk->v)) {
                    tk->k = TK_TYPE;
                    break;
                }
                break;
        }
    }
}


LexerOut* LuaLexer_Generate(char *in) {
    LexerOut* lo = LuaMem_Alloc(sizeof(LexerOut));
    
    LexState ls = {
        .tk_buf = {
            .buffer = LuaMem_Zeroalloc(64),
            .siz = 0,
            .cap = 64, 
        },
        
        .tklst = {
            .tks = LuaMem_Alloc(sizeof(void*)*LEXER_TKLST_INIT_CAP),
            .siz = 0,
            .cap = LEXER_TKLST_INIT_CAP,
        },

        .in_string = false,
        .string_clause = '"',

        .line = 1,
        .column = 1,
    };

    source_look(&ls, in);


    /* setup the LexerOut */
    lo->siz = ls.tklst.siz;
    lo->tks = LuaMem_Alloc(sizeof(void*)*ls.tklst.siz);

    for (int i = 0; i < ls.tklst.siz; i++)
        lo->tks[i] = ls.tklst.tks[i];
    
    LuaMem_Free(ls.tk_buf.buffer);
    LuaMem_Free(ls.tklst.tks);
    return lo;
}

void LuaLexer_Free(LexerOut* lo) {
   for (int i = 0; i < lo->siz; i++) {
        LuaMem_Free(lo->tks[i]);
   } 
   LuaMem_Free(lo->tks);
   LuaMem_Free(lo);
}

void LuaLexer_Print(LexerOut* lo) {
    for (int i = 0; i < lo->siz; i++) {
        printf("kind: %d. value: %s\n", lo->tks[i]->k, lo->tks[i]->v);
    }
}

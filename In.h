#pragma once
#include <stdio.h>
#include <stdbool.h>

/* lmem.c */
void* _LuaMem_Alloc(size_t siz, char* file, int line);
void* _LuaMem_Realloc(void* block, size_t siz, char* file, int line);
void* _LuaMem_Calloc(size_t amnts, size_t siz, char* file, int line);
void _LuaMem_Free(void* block, char* file, int line);

#define LuaMem_Alloc(siz) _LuaMem_Alloc(siz, __FILE__, __LINE__)
#define LuaMem_Realloc(block, siz) _LuaMem_Realloc(block, siz, __FILE__, __LINE__)
#define LuaMem_Free(block) _LuaMem_Free(block, __FILE__, __LINE__)
#define LuaMem_Calloc(amnts, siz) _LuaMem_Calloc(amnts, siz, __FILE__, __LINE__)
#define LuaMem_Zeroalloc(siz) _LuaMem_Calloc(siz, 1, __FILE__, __LINE__)

/* lutil.c */
typedef struct PtrVec {
    void** items;
    size_t siz;
    size_t cap;

    void* tail;
} PtrVec;

PtrVec* LuaUtil_Init_PtrVec(void);
void LuaUtil_PtrVec_Push(PtrVec* pv, void* i);
void LuaUtil_PtrVec_Pop(PtrVec* pv);
void LuaUtil_Free_PtrVec(PtrVec* pv);

/* llexer.c */
typedef enum TKKind {
    TK_IDENTIFIER,
    TK_STRING,
    TK_NUMERIC,
    TK_KEYWORD,
    TK_GLOBAL,
    TK_BOOLEAN,
    TK_TYPE,

    TK_OPEN_PAREN,
    TK_CLOSE_PAREN,
    TK_SEMICOLON,
    TK_COLON,
    TK_QUOTE,

} TKKind;

typedef struct Token {
    TKKind k;
    char* v;

    struct Token* prev;
    struct Token* next;

    int line;
    int column;
} Token;

typedef struct LexerOut {
    Token** tks; /* all the tokens */
    size_t siz; /* list size */
} LexerOut;

typedef struct LexState {
    struct {
        char* buffer;
        size_t siz;
        size_t cap;
    } tk_buf;

    struct {
        Token** tks;
        size_t siz;
        size_t cap;
    } tklst;

    bool in_string;
    char string_clause;

    int line;
    int column;
} LexState;

LexerOut* LuaLexer_Generate(char* in);
void LuaLexer_Free(LexerOut* lo);
void LuaLexer_Print(LexerOut* lo);

/* lparse.c */
typedef enum NDKind {
    ND_NULL, /* root */
    ND_IDENTIFIER,
    ND_STRING_LITERAL,
    ND_NUMERIC_LITERAL,
    ND_GENERIC_KEYWORD, /* keyword with no special semantics */
    ND_GENERIC_GLOBAL,
    ND_BOOLEAN_LITERAL,
    ND_TYPE,

    /* declarations */
    ND_VARIABLE_DECLARATION,
    ND_FUNCTION_DECLARATION,

    /* statements */
    ND_CALL_STATEMENT,


    /* expressions */

} NDKind;

typedef struct Node {
    NDKind k;
    char* v;

    struct Node* prev;
    struct {
        struct Node** r; // refs/data (main)
        size_t s; // siz
        size_t c; // cap
    } next;

    struct {
        int line;
        int column;
    } m; /* misc: passed over from token; not useful for processing */
} Node;

typedef enum SKind {
    SCOPE_NULL,
    SCOPE_CLAUSE,
    SCOPE_CALL,
} SKind;

typedef struct Scope {
    SKind k;
    Node* nd;
} Scope;

typedef struct ParseState {
    PtrVec* scopes;
    PtrVec* scope_archive;
    Node* next;
    bool set_next_to_scope;

    struct {
        Node** nds;
        Node* r;
        size_t s;
        size_t c;
    } ndlst;

} ParseState;

typedef struct ParseOut {
    Node** nds;
    size_t siz;
    Node* root;
} ParseOut;

ParseOut* LuaParse_Generate_Tree(LexerOut* lo);
void LuaParse_Free_Tree(ParseOut* po);
void LuaParse_Print_Tree(ParseOut* po);

/* lop.c */
typedef enum OPCMDKind {
    NO_COMMAND,
    WRITE_STDBUF,
    CALL,
    EXIT_PROGRAM,

} OPCMDKind;

typedef struct OPCodeLine {
    PtrVec* prms; // char**
    OPCMDKind cmd;
} OPCodeLine;

typedef struct OPState {
    PtrVec* opls; // OpCodeLine**
    
    struct {
        OPCMDKind cmd;
        PtrVec* prms;
    } line; // current line state
} OPState;

// ptrvec of OPCodeLine's
PtrVec* LuaOP_Generate(ParseOut* po);
void LuaOP_Free_Output(PtrVec* pv);

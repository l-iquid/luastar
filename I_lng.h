#pragma once
#include <stdio.h>
#include <stdbool.h>

/* utils */
typedef struct PtrVec {
  void** data;
  size_t siz;
  size_t cap;
} PtrVec;

PtrVec* L_init_ptrvec();
void L_ptrvec_push(PtrVec* x, void* item);
void L_ptrvec_pop(PtrVec* x);
#define L_free_ptrvec(x) free(x->data); free(x);


/* Safe allocation methods. */

/* Use this union if you want. */
typedef union complexalloc {
  void* vptr;
  char* chrptr;
} complexalloc;

void* L_init_mem(size_t siz, unsigned long line, void* fil);
void* L_mem_realloc(void* block, size_t siz, unsigned long line, void* fil);
void* L_init_calloc(size_t siz, size_t keysiz, unsigned long line, void* fil);
void L_mem_free(void* block, unsigned long line, void* fil);

/* Use these macros (for ease of use). */

#define L_alloc(siz) L_init_mem(siz, __LINE__, __FILE__)
#define L_calloc(siz,r) L_init_calloc(siz, r, __LINE__, __FILE__)
#define L_zeroalloc(siz) L_calloc(siz, 1) /* Just a simpler version of calloc. */
#define L_realloc(block,siz) L_mem_realloc(block, siz, __LINE__, __FILE__)
#define L_free(block) L_mem_free(block, __LINE__, __FILE__)

/* String functions. */

char* L_stdp(char* z);
int L_stcmp(const char* x, const char* y);
int L_stln(char* x);

/* lexer */

typedef enum TokenKind {
  TK_IDENTIFIER,
  TK_NUMERIC,
  TK_STRING,
  TK_BOOLEAN,
  TK_KEYWORD,
  TK_LST_TYPE, // built in type
  TK_CUSTOM_TYPE,
  TK_CALL,

  // symbols
  TK_SEMICOLON,
  TK_COLON,
  TK_LEFT_PAREN,
  TK_RIGHT_PAREN,
  TK_EQUALS,
  TK_EQUALSEQUALS,
  TK_ADD,
  TK_SUB,
  TK_MUL,
  TK_DIV,
  TK_INCREMENT,
  TK_DECREMENT,
  TK_MULTEMENT,
  TK_DIVEMENT,
  TK_LEFT_BRACKET,
  TK_RIGHT_BRACKET,
  TK_QUOTE,
  TK_COMMA,
  TK_GREATER,
  TK_LESS,
  TK_GREATER_OR_EQ,
  TK_LESS_OR_EQ,
  TK_LEFT_SQUIRLY,
  TK_RIGHT_SQUIRLY,
  TK_AMPERSAND,
  
} TokenKind;

typedef struct Token {
  TokenKind kind;
  char* value; // malloc/strdup

  struct Token* prev;
  struct Token* next;

  unsigned int line;
  unsigned int column;
} Token;

typedef struct TokenList {
  Token** data;
  size_t siz;
  size_t cap;
} TokenList;

typedef struct LexState {
  struct {
    char* buffer;
    size_t siz;
    size_t cap;
  } tkbuf;

  int line;
  int column;
  char string_clause_type;
  bool in_string;

  bool is_comment_multiline;
  bool in_comment;
} LexState;

typedef struct LexErrchkState {
  int parenthesis_amnt;
  int last_paren_line;
  int last_paren_column;
} LexErrchkState;

TokenList* L_generate_tokens(char* input);
void L_free_tklst(TokenList* x);
void L_print_tklst(TokenList* x);

/* parser */
typedef enum NodeKind {
  NODE_NULL,
  NODE_IDENTIFIER,
  NODE_NUMERIC_LITERAL,
  NODE_STRING_LITERAL,
  NODE_BOOLEAN_LITERAL,
  NODE_KEYWORD,
  NODE_TYPE,
  NODE_CALL,

  NODE_GENERIC_SCOPE,
  NODE_GENERIC_SCOPE_END,

  /* statements */
  NODE_VAR_DECL_STATEMENT,
  NODE_IF_STATEMENT,

} NodeKind;

typedef enum ScopeKind {
  SCOPE_NULL,
  SCOPE_CLAUSE,
  SCOPE_CALL,
  SCOPE_GENERIC
} ScopeKind;

typedef struct Node {
  NodeKind kind;
  char* value;

  struct Node* prev;
  struct {
    struct Node** refs;
    size_t siz;
    size_t cap;
  } next;

  int line;
  int column;
} Node;

typedef struct Scope {
  Node* nd;
  ScopeKind kind;
} Scope;

typedef struct AbstractSyntaxTree {
  Node** nodes;
  size_t siz;
  size_t cap;

  Node* root;
} AbstractSyntaxTree;

typedef struct ParseState {
  PtrVec* scopes;
  struct {
    Scope* data;
    size_t siz;
    size_t cap;
  } scope_archive;

  Node* next;
} ParseState;

AbstractSyntaxTree* L_generate_ast(TokenList* tklst);
void L_free_ast(AbstractSyntaxTree* tree);
void L_print_ast(AbstractSyntaxTree* tree);

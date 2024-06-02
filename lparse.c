#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "I_lng.h"


/* config */
#define AST_INIT_CAPACITY 16
#define AST_REALLOC_CAP   8


/**************************/
/*** AST NODE FUNCTIONS ***/
/**************************/
static inline void set_next(Node* parent, Node* child) {
  if (parent->next.siz + 1 >= parent->next.cap) {
    parent->next.cap += 2;
    parent->next.refs = L_realloc(parent->next.refs, sizeof(void*)*parent->next.cap);
  }

  parent->next.refs[parent->next.siz] = child;
  parent->next.siz++;
}


static Node* create_node(Token* tk, NodeKind kind, Node* prev) {
  Node* nd = L_alloc(sizeof(Node));
  nd->kind = kind;
  nd->value = L_stdp(tk->value);
  nd->line = tk->line;
  nd->column = tk->column;
  nd->prev = prev;

  if (prev) {
    set_next(prev, nd);
  }

  return nd;
}

/* Inserts a newly created node into the tree. */
static inline void insert_node(AbstractSyntaxTree* tree, Node* nd) {
  if (tree->siz + 1 >= tree->cap) {
    tree->cap += AST_REALLOC_CAP;
    tree->nodes = L_realloc(tree->nodes, sizeof(void*)*tree->cap);
  }

  tree->nodes[tree->siz] = nd;
  tree->siz++;
}

#define push_node(tk, kind, prev) {Node* nod = create_node(tk, kind, prev); insert_node(tree, nod);}
#define access_last_node(tree) tree->nodes[tree->siz-1]
#define set_next(tree, ps) ps->next = access_last_node(tree)

/***************************/
/***** SCOPE FUNCTIONS *****/
/***************************/
/* Creates a scope into the archive. */
static void create_scope(ParseState* ps, ScopeKind kind, Node* nd) {
  if (ps->scope_archive.siz + 1 > ps->scope_archive.cap) {
    ps->scope_archive.cap += 2;
    ps->scope_archive.data = L_realloc(ps->scope_archive.data, sizeof(Scope)*ps->scope_archive.cap);
  }

  ps->scope_archive.data[ps->scope_archive.siz] = (Scope) {
    .kind = kind,
    .nd = nd
  };
  ps->scope_archive.siz++;
}


#define new_scope(ps,kind,nd) create_scope(ps,kind,nd); L_ptrvec_push(ps->scopes,&ps->scope_archive.data[ps->scope_archive.siz-1])
#define pop_scope(ps) L_ptrvec_pop(ps->scopes) /* FYI this shouldn't pop from scope_archive, it's kept for error checks. */
#define access_last_scope(ps) ((Scope*)ps->scopes->data[ps->scopes->siz-1])

/*
* The main code.
*/
/* Token evaluators. */

static inline void identifier(AbstractSyntaxTree* tree, ParseState* ps, Token* tk) {
  push_node(tk, NODE_IDENTIFIER, ps->next);
  set_next(tree, ps);
}
static inline void literal(AbstractSyntaxTree* tree, ParseState* ps, Token* tk) {
  switch (tk->kind) {
    case TK_STRING:
      push_node(tk, NODE_STRING_LITERAL, ps->next);
      break;
    case TK_NUMERIC:
      push_node(tk, NODE_NUMERIC_LITERAL, ps->next);
      break;
    case TK_BOOLEAN:
      push_node(tk, NODE_BOOLEAN_LITERAL, ps->next);
      break;
  }
  set_next(tree, ps);
}
static inline void keyword(AbstractSyntaxTree* tree, ParseState* ps, Token* tk) {
  if (L_stcmp(tk->value, "local")) {
    /*
    local
      => type
      => =
        => value
    */
    push_node(tk, NODE_VAR_DECL_STATEMENT, ps->next);
    set_next(tree, ps);
    return;
  }
  if (L_stcmp(tk->value, "if")) {
    /*
    if
      => condition
      => statement 1
      => statement 2
      ...
    */
    push_node(tk, NODE_IF_STATEMENT, ps->next);
    set_next(tree, ps);
    new_scope(ps, SCOPE_CLAUSE, access_last_node(tree));
    return;
  }
}
/* : */
static inline void type_denoter(AbstractSyntaxTree* tree, ParseState* ps, Token* tk) {
  
  switch (tk->next->kind) {
    case TK_CUSTOM_TYPE:
    case TK_LST_TYPE:
      push_node(tk->next, NODE_TYPE, ps->next);
      break;

    default:
      /* error */
      break;
  }

}
static inline void function_call(AbstractSyntaxTree* tree, ParseState* ps, Token* tk) {
  push_node(tk, NODE_CALL, ps->next);
  set_next(tree, ps);
  new_scope(ps, SCOPE_CALL, access_last_node(tree));
}
static inline void left_paren(AbstractSyntaxTree* tree, ParseState* ps, Token* tk) {
  switch (tk->prev->kind) {
    case TK_KEYWORD:
    case TK_CALL:
      break;

    default:
      push_node(tk, NODE_GENERIC_SCOPE, ps->next);
      set_next(tree, ps);
      new_scope(ps, SCOPE_GENERIC, access_last_node(tree));
      break;
  }
}
static inline void right_paren(AbstractSyntaxTree* tree, ParseState* ps, Token* tk) {
  switch (access_last_scope(ps)->kind) {
    case SCOPE_CLAUSE:
      break;

    case SCOPE_GENERIC:
      push_node(tk, NODE_GENERIC_SCOPE_END, ps->next);
      pop_scope(ps);
      break;
    
    default:
      pop_scope(ps);
      break;
  }
}
static inline void comma(AbstractSyntaxTree* tree, ParseState* ps, Token* tk) {
  switch (access_last_scope(ps)->kind) {
    case SCOPE_CALL:
      ps->next = access_last_scope(ps)->nd;
      break;
  }
}
static inline void left_squirly(AbstractSyntaxTree* tree, ParseState* ps, Token* tk) {
  switch (access_last_scope(ps)->kind) {
    case SCOPE_CLAUSE:
      ps->next = access_last_scope(ps)->nd;
      break;
  }
}
static inline void right_squirly(AbstractSyntaxTree* tree, ParseState* ps, Token* tk) {
  switch (access_last_scope(ps)->kind) {
    case SCOPE_CLAUSE:
      pop_scope(ps);
      ps->next = access_last_scope(ps)->nd;
      break;
  }
}

/* Main function */
static inline void analyze_tokens(AbstractSyntaxTree* tree, ParseState* ps, TokenList* tklst) {

  for (int i = 0; i < tklst->siz; i++) {
    Token* tk = tklst->data[i];

    switch (tk->kind) {
      case TK_QUOTE:
      case TK_LST_TYPE:
      case TK_CUSTOM_TYPE:
        break;

      case TK_COMMA:
        comma(tree, ps, tk);
        break;

      case TK_LEFT_PAREN:
        left_paren(tree, ps, tk);
        break;
      case TK_RIGHT_PAREN:
        right_paren(tree, ps, tk);
        break;
      case TK_LEFT_SQUIRLY:
        left_squirly(tree, ps, tk);
        break;
      case TK_RIGHT_SQUIRLY:

        break;

      case TK_COLON:
        type_denoter(tree, ps, tk);
        break;

      case TK_STRING:
      case TK_NUMERIC:
      case TK_BOOLEAN:
        literal(tree, ps, tk);
        break;

      case TK_SEMICOLON:
        ps->next = access_last_scope(ps)->nd;
        break;

      case TK_CALL:
        function_call(tree, ps, tk);
        break;

      case TK_KEYWORD:
        keyword(tree, ps, tk);
        break;

      default:
        identifier(tree, ps, tk);
        break;
    }
  }

}


/*
API.
*/

/*
* The main function.
*/
AbstractSyntaxTree* L_generate_ast(TokenList* tklst) {
  AbstractSyntaxTree* tree = L_alloc(sizeof(AbstractSyntaxTree));
  tree->nodes              = L_alloc(sizeof(void*)*AST_INIT_CAPACITY);
  tree->cap = AST_INIT_CAPACITY;
  tree->siz = 0;

  /* Create root node. */
  push_node(&(Token){.value=""}, NODE_NULL, NULL);
  tree->root = tree->nodes[0];


  ParseState ps = {
    .scopes = L_init_ptrvec(),
    .scope_archive = {
      .data = L_alloc(sizeof(Scope)*4),
      .siz = 0,
      .cap = 4
    },

    .next = tree->root,
  };

  /* It is the bottom scope. */
  new_scope((&ps), SCOPE_NULL, tree->root);


  analyze_tokens(tree, &ps, tklst);


  L_free_ptrvec(ps.scopes);
  L_free(ps.scope_archive.data);
  return tree;
}


void L_free_ast(AbstractSyntaxTree* tree) {
  for (int i = 0; i < tree->siz; i++) {
    Node* nd = tree->nodes[i];
    L_free(nd->value);
    L_free(nd);
  }
  L_free(tree->nodes);
  L_free(tree);
}

static inline void print_node(Node* nd, int layer) {
  for (int i = 0; i < layer; i++) {
    printf("  ");
  }
  printf(">> kind: %d. value: %s\n", nd->kind, nd->value);

  for (int i = 0; i < nd->next.siz; i++) {
    print_node(nd->next.refs[i], layer+1);
  }
}

void L_print_ast(AbstractSyntaxTree* tree) {
  printf("=====AST=====\n");
  print_node(tree->root, 0);
  printf("=============\n");
}

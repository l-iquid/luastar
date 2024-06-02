#include <stdio.h>
#include <stdlib.h>
#include "I_lng.h"

#define arrlen(l) sizeof(l)/sizeof(l[0])

/* Configs. */
#define TKLST_INIT_CAP 16
#define TKLST_REALLOC  8

static const char* lkeywords[] = {
  "local", "if"
};
static const char* ltypes[] = {
  "string"
};
static const char* lglobfuncs[] = {
  "print"
};




static void lex_panic(char* msg) { /* Internal panic. Not user related. */
  fprintf(stderr, msg);
  exit(EXIT_FAILURE);
}

/* basic ahh error checking atm */
static void lexer_error(char* msg, int line, int column) {
  fprintf(stderr, "llexer.c: :%d:%d: %s\n", line, column, msg);
  exit(EXIT_FAILURE);
}

/* Just a shortened version of lexer_error. */
#define lex_err(msg) lexer_error(msg, ls->line, ls->column);


/*************************/
/**** TOKEN FUNCTIONS ****/
/*************************/

/*
How every token is created.
*/
static inline Token* make_token(TokenKind kind, char* value, int line, int column) {
  Token* x = L_alloc(sizeof(Token));
  x->kind = kind;
  x->value = L_stdp(value);
  x->line = line;
  x->column = column;
  return x;
}


/* Reallocation check. */
static inline void tklst_realloc_chk(TokenList* tklst) {
  if (tklst->siz + 1 >= tklst->cap) {
    tklst->cap += TKLST_REALLOC;
    tklst->data = L_realloc(tklst->data, sizeof(void*)*tklst->cap);
  }
}

/* Insert tk into tokenlist (tk must be a pointer). */
#define tklst_new(tklst, tk) tklst_realloc_chk(tklst); tklst->data[tklst->siz] = tk; tklst->siz++; \
  if (tklst->siz > 1) tklst->data[tklst->siz-2]->next = tklst->data[tklst->siz-1]; tklst->data[tklst->siz-1]->prev = tklst->data[tklst->siz-2];


/*************************/
/* LEXER STATE FUNCTIONS */
/*************************/
/* Pushes one character to ls->tkbuf.buffer. */
static void ls_push_chr(LexState* ls, char CHR) {
  if (ls->tkbuf.siz + 1 >= ls->tkbuf.cap) {
    ls->tkbuf.cap += 16;
    ls->tkbuf.buffer = L_realloc(ls->tkbuf.buffer, ls->tkbuf.cap);

    for (int i = ls->tkbuf.cap-16; ls->tkbuf.cap; i++) {
      ls->tkbuf.buffer[i] = 0;
    }
  }
  ls->tkbuf.buffer[ls->tkbuf.siz] = CHR;
  ls->tkbuf.siz++;
}

/* Pushes a whole string to ls->tkbuf.buffer (without null-terminator). */
#define ls_push_str(ls, str) for (int i = 0; i < L_stln(str); i++) ls_push_chr(ls, str[i]);

/* Sets ls->tkbuf.buffer contents to nothing. */
static inline void ls_clr_buf(LexState* ls) {
  for (int i = 0; i < ls->tkbuf.siz; i++) {
    ls->tkbuf.buffer[i] = 0;
  }
  ls->tkbuf.siz = 0;
}

static inline void push_tk_from_buf(TokenList* tklst, LexState* ls, TokenKind kind) {
  Token* tk = make_token(kind, ls->tkbuf.buffer, ls->line, ls->column);
  tklst_new(tklst, tk);
  ls_clr_buf(ls);
}

/* push_tk_from_buf but with a check. */
#define ls_push_tk(tklst, ls) if (ls->tkbuf.siz > 0) {push_tk_from_buf(tklst, ls, ls->in_string ? TK_STRING : TK_IDENTIFIER);}

/* Only for switches/loops. */
#define cmnt_chk() if (ls->in_comment) break;


/* Symbol macros. */

#define _onechr_str_chk(ls,value) if (ls->in_string) {ls_push_chr(ls, value[0]); break;} /* if in string then it'll be a regular char */
/* Used for a single character but can be multiple. */
#define tk_onechr(kind, value)                                                      \
  cmnt_chk();                                                                       \
  if (kind != TK_QUOTE) _onechr_str_chk(ls, value);                                 \
  ls_push_tk(tklst, ls); ls_push_str(ls, value); push_tk_from_buf(tklst, ls, kind);


/* The last character must be '='. */
#define tk_twochr(kind_1, value_1, kind_2, value_2) \
  cmnt_chk();                                       \
  switch (input[1]) {                               \
    case '=':                                       \
      tk_onechr(kind_2, value_2);                   \
      *input++;                                     \
      break;                                        \
    default:                                        \
      tk_onechr(kind_1, value_1);                   \
      break;                                        \
  }


/* Some other macros (for readability). */
#define is_num(chr) (chr >= '0' && chr <= '9')
/* If a regular character is a valid one or not (strings will not use these checks). */
#define is_valid_input(in) ((in >= 'A' && in <= 'Z') || (in >= 'a' && in <= 'z') || is_num(in))

/* One character to a null-terminated string. */
#define chr_to_str(chr) ((char[2]){chr, '\0'})



/*
Final error checking stage.
*/
static inline void lex_errchk_final(TokenList* tklst, LexState* ls) {
  if (tklst->siz < 1) return;
  if (ls->in_string) {
    lex_err("No end to string bound.");
  }

  LexErrchkState es = {
    .parenthesis_amnt = 0,
    .last_paren_line = 1,
    .last_paren_column = 1
  };

  for (int i = 0; i < tklst->siz; i++) {
    Token* tk = tklst->data[i];

    switch (tk->kind) {
      case TK_LEFT_PAREN:
      case TK_RIGHT_PAREN:
        es.parenthesis_amnt++;
        es.last_paren_line = tk->line;
        es.last_paren_column = tk->column;
        break;

      
    }
  }

  if (es.parenthesis_amnt % 2 == 1) {
    /* Odd number of parenthesis (indicates there's no end to a clause). */
    lexer_error("No end to parenthesis clause.", es.last_paren_line, es.last_paren_column);
  }
}

/* Every line there's error checking. */
static inline void newline_errchk(TokenList* tklst, LexState* ls) {
  if (ls->in_string) {
    lex_err("No end to string bound.");
  }
}

/*
The source analyzer (the main code).
*/
static inline void analyze_inputs(TokenList* tklst, char* lines, LexState* ls) {
  char* input = lines;

  /* text analysis stage */
  while (*input != '\0') {
    switch (*input) {
      case ':': tk_onechr(TK_COLON, ":"); break;
      case ';': tk_onechr(TK_SEMICOLON, ";"); break;
      case '(': tk_onechr(TK_LEFT_PAREN, "("); break;
      case ')': tk_onechr(TK_RIGHT_PAREN, ")"); break;
      case '{': tk_onechr(TK_LEFT_SQUIRLY, "{"); break;
      case '}': tk_onechr(TK_RIGHT_SQUIRLY, "}"); break;
      case ',': tk_onechr(TK_COMMA, ","); break;
      case '=': tk_twochr(TK_EQUALS, "=", TK_EQUALSEQUALS, "=="); break;
      case '&': tk_onechr(TK_AMPERSAND, "&"); break;
      case '+': tk_twochr(TK_ADD, "+", TK_INCREMENT, "+="); break;
      case '-':
        switch (input[1]) {
          case '-':
            /* is a comment */
            ls->in_comment = true;

            switch (input[2]) {
              case '[':
                switch (input[3]) {
                  case '[':
                    /* is multiline */
                    ls->is_comment_multiline = true;
                    *input += 2;
                    break;
                }
                break;
            }

            *input++;
            break;

          default:
            tk_twochr(TK_SUB, "-", TK_DECREMENT, "-=");
            break;
        }
        break;
      case '*': tk_twochr(TK_MUL, "*", TK_MULTEMENT, "*="); break;
      case '/': tk_twochr(TK_DIV, "/", TK_DIVEMENT, "/="); break;

      case '[': tk_onechr(TK_LEFT_BRACKET, "["); break;
      case ']':
        switch (input[1]) {
          case ']':
            if (ls->is_comment_multiline && ls->in_comment) {
              /* end multiline comment */
              ls->in_comment = false;
              *input++;
            }
            break;

          default:
            tk_onechr(TK_RIGHT_BRACKET, "]");
            break;
        }
        break;

      case '\'':
      case '"':
        cmnt_chk();
        if (ls->in_string && ls->string_clause_type == *input) {
          // end string
          tk_onechr(TK_QUOTE, chr_to_str(*input));
          ls->in_string = false;
        } else {
          if (ls->in_string) {
            ls_push_chr(ls, *input);
          } else {
            // start string
            tk_onechr(TK_QUOTE, chr_to_str(*input));
            ls->in_string = true;
            ls->string_clause_type = *input;
          }
        }
        break;

      case 10: /* new line */
        if (ls->in_comment && ls->is_comment_multiline == false) {
          ls->in_comment = false;
        }

        newline_errchk(tklst, ls);
        ls->line++;
        ls->column = 0;
      case ' ':
        cmnt_chk();
        if (ls->in_string) {
          ls_push_chr(ls, *input);
        } else {
          ls_push_tk(tklst, ls);
        }
        break;

      default: /* every other character out there */
        cmnt_chk();
        if (ls->in_string == false && is_valid_input(*input)) {
          ls_push_chr(ls, *input);
        } else {
          if (ls->in_string == false) {
            /* invalid character error */
            char buf[64];
            snprintf(buf, 64, "Invalid character '%c'.", *input);
            lex_err(buf);
          } else {
            ls_push_chr(ls, *input);
          }
        }
        break;
    }

    ls->column++;
    *input++;
  }

  ls_push_tk(tklst, ls);


  /* refine the tokens */
  for (int i = 0; i < tklst->siz; i++) {
    Token* tk = tklst->data[i];

    switch (tk->kind) {
      case TK_IDENTIFIER:
        if (is_num(tk->value[0])) {
          tk->kind = TK_NUMERIC;
          break;
        }

        for (int i = 0; i < arrlen(lkeywords); i++) {
          if (L_stcmp(tk->value, lkeywords[i])) {
            tk->kind = TK_KEYWORD;
          }
        }
        for (int i = 0; i < arrlen(lglobfuncs); i++) {
          if (L_stcmp(tk->value, lglobfuncs[i])) {
            tk->kind = TK_CALL;
          }
        }
        for (int i = 0; i < arrlen(ltypes); i++) {
          if (L_stcmp(tk->value, ltypes[i])) {
            tk->kind = TK_LST_TYPE;
          }
        }
        break;
    }
  }

  newline_errchk(tklst, ls);
  lex_errchk_final(tklst, ls);
}


/*
API.
*/

/*
The main function of the lexer.
*/
TokenList* L_generate_tokens(char* input) {
  TokenList* tklst = L_alloc(sizeof(TokenList));
  tklst->data      = L_alloc(sizeof(void*)*TKLST_INIT_CAP);
  tklst->cap = TKLST_INIT_CAP;
  tklst->siz = 0;

  LexState ls = {
    .tkbuf = {
      .buffer = L_zeroalloc(64),
      .siz = 0,
      .cap = 64,
    },

    .line = 1,
    .column = 1,
    .string_clause_type = '"',
    .in_string = false,
  };

  analyze_inputs(tklst, input, &ls);

  L_free(ls.tkbuf.buffer);
  return tklst;
}

void L_free_tklst(TokenList* x) {
  for (int i = 0; i < x->siz; i++) {
    Token* tk = x->data[i];
    L_free(tk->value);
    L_free(tk);
  }
  L_free(x->data);
  L_free(x);
}

void L_print_tklst(TokenList* x) {
  for (int i = 0; i < x->siz; i++) {
    Token* tk = x->data[i];
    printf("kind: %d. value: %s. :%d:%d:\n", tk->kind, tk->value, tk->line, tk->column);
  }
}


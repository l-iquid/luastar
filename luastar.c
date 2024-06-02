#include <stdio.h>
#include <stdlib.h>
#include "luastar.h"
#include "I_lng.h"

void lst_test_compile_input(char* input) {
  TokenList* tokens = L_generate_tokens(input);
  L_print_tklst(tokens);
  
  AbstractSyntaxTree* tree = L_generate_ast(tokens);

  L_free_tklst(tokens);

  L_print_ast(tree);

  L_free_ast(tree);
}
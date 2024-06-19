#include <stdio.h>
#include <stdlib.h>
#include "In.h"
#include "luastar.h"

#define PRINT_LEXER 1
#define PRINT_TREE  1

void LuaEntry_Test_Inputs(char* in) {
    LexerOut* tks = LuaLexer_Generate(in);
    LuaLexer_Print(tks);

    ParseOut* po = LuaParse_Generate_Tree(tks);
    LuaParse_Print_Tree(po);

    PtrVec* ops = LuaOP_Generate(po);

    LuaLexer_Free(tks);
    LuaParse_Free_Tree(po);
    LuaOP_Free_Output(ops);
}

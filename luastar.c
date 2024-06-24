#include <stdio.h>
#include <stdlib.h>
#include "In.h"
#include "luastar.h"

#define PRINT_LEXER 0
#define PRINT_TREE  1

void LuaEntry_Test_Inputs(char* in) {
    LexerOut* tks = LuaLexer_Generate(in);
    #if(PRINT_LEXER == 1)
        LuaLexer_Print(tks);
    #endif
    ParseOut* po = LuaParse_Generate_Tree(tks);
    #if(PRINT_TREE == 1)
        LuaParse_Print_Tree(po);
    #endif
    //PtrVec* ops = LuaOP_Generate(po);
    
    PtrVec* go = LuaGen_x86_64_Linux(po);

    FILE* f = fopen("bin/a.s", "w");
    for (int i = 0; i < go->siz; i++) {
        fprintf(f, "%s\n", (char*)go->items[i]);
    }
    fclose(f);

    LuaLexer_Free(tks);
    LuaParse_Free_Tree(po);
    LuaGen_Free(go);

    printf("===OUTPUT===\n");
    system("gcc -c bin/a.s && ld a.o && ./a.out");
    system("rm a.out && rm a.o");
    printf("====STOP====\n");
    //LuaOP_Free_Output(ops);
}

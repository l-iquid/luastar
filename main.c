/*
 * Command line entry point for Lua*.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "luastar.h"


/* CLI */
#define CLI_START_MSG   "The Lua* programming language. A fast, compiled, general-purpose language.\n"
#define CLI_START_MSG_2 "Type 'HELP' for more information.\n"
#define CLI_HELP_MSG    "'RUN' -- runs the inputs, 'EXIT' -- exits the program.\n"


void LuaMain_CLI(void) {
    char input_buf[256];
    char* inputs = malloc(128);
    size_t inputs_len = 128;
    size_t inputs_siz = 0;

    printf(CLI_START_MSG);
    printf(CLI_START_MSG_2);

    for (;;) {
        printf(">>> ");
        fgets(input_buf, sizeof(input_buf), stdin);

        if (strcmp(input_buf, "EXIT\n") == 0) {
            exit(EXIT_SUCCESS);
        }
        if (strcmp(input_buf, "HELP\n") == 0) {
            printf(CLI_HELP_MSG);
        }
        if (strcmp(input_buf, "RUN\n") == 0) {
            LuaEntry_Test_Inputs(inputs);

            for (int i = 0; i < inputs_siz; i++) {
                inputs[i] = 0;
            }
            inputs_siz = 0;
            continue;
        }


        for (int i = 0; i < strlen(input_buf); i++) {
            if (inputs_siz + 1 > inputs_len) {
                inputs_len += 64;
                inputs = realloc(inputs, inputs_len);
            }
            inputs[inputs_siz] = input_buf[i];
            inputs_siz++;
        }
    }
}



/* ENTRY */
int main(int argc, char* argv[]) {
    if (argc < 2) {
        // CLI mode -- no arguments passed
        LuaMain_CLI();
    }



    return EXIT_SUCCESS;
}

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "luastar.h"

#define CLI_MODE  1

/*
The compilation process of the file.
*/

/* Reads file. */
static void process_file(const char* filename) {
  FILE* f = fopen(filename, "r");

  fseek(f, 0, SEEK_END);
  size_t len = ftell(f);
  fseek(f, 0, SEEK_SET);

  char* contents = malloc(len+1);

  fread(contents, 1, len, f);
  contents[len] = '\0';

  lst_test_compile_input(contents);

  fclose(f);
  free(contents);
}

/*
Main entry point.
*/
int main(int argc, char* argv[]) {
  if (CLI_MODE) {
    // idk later
  }

  if (argc < 2) {
    fprintf(stderr, "COMMAND ARG AMNT IS LESS THAN 2.\n");
    exit(EXIT_FAILURE);
  }

  const char* filename = argv[1];

  if (access(filename, F_OK)) {
    fprintf(stderr, "FILE '%s' DOESN'T EXIST.\n", filename);
    exit(EXIT_FAILURE);
  }

  process_file(filename);

  return EXIT_SUCCESS;
}
#include <stdio.h>
#include <stdlib.h>
#include "I_lng.h"

void* L_init_mem(size_t siz, unsigned long line, void* fil) {
  void* x = malloc(siz);
  if (x == NULL) {
    fprintf(stderr, "[%s:%d] Failed to allocate [%d] bytes.\n", fil, line, siz);
    exit(EXIT_FAILURE);
  }
  return x;
}
void* L_init_calloc(size_t siz, size_t keysiz, unsigned long line, void* fil) {
  complexalloc x = (complexalloc)L_init_mem(siz, line, fil);
  for (int i = 0; i < siz*keysiz; i++) {
    x.chrptr[i] = 0;
  }
  return x.vptr;
}
void* L_mem_realloc(void* block, size_t siz, unsigned long line, void* fil) {
  void* x = realloc(block, siz);
  if (x == NULL) {
    fprintf(stderr, "[%s:%d] Failed to reallocate [%d] bytes.\n", fil, line, siz);
    exit(EXIT_FAILURE);
  }
  return x;
}
void L_mem_free(void* block, unsigned long line, void* fil) {
  if (block == NULL) {
    fprintf(stderr, "[%s:%d] Unable to free block. Block already freed.\n", fil, line);
    return;
  }
  free(block);
  block = NULL;
}
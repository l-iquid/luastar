#include <stdio.h>
#include <stdlib.h>
#include "I_lng.h"

/* Creates new ptrvec. */
PtrVec* L_init_ptrvec() {
  PtrVec* x = L_alloc(sizeof(PtrVec));
  x->data = L_alloc(sizeof(void*)*8);
  x->cap = 8;
  x->siz = 0;
  return x;
}
void L_ptrvec_push(PtrVec* x, void* item) {
  if (x->siz + 1 > x->cap) {
    x->cap += 8;
    x->data = L_realloc(x->data, sizeof(void*)*x->cap);
  }

  x->data[x->siz] = item;
  x->siz++;
}
void L_ptrvec_pop(PtrVec* x) {
  x->data[x->siz-1] = NULL;
  x->siz--;
}


#pragma GCC diagnostic ignored "-Wdiscarded-qualifiers" /* shut the fuck up compiler */

/* String length. */
int L_stln(char* x) {
  int i = -1;
  while (x[++i]!='\0');
  return i;
}

/* String duplication. Must free() after use. */
char* L_stdp(char* z) {
  int len = L_stln(z);
  char* x = L_alloc(len+1);
  if (x==NULL)return 0;
  x[len] = '\0';

  int i = -1;
  while (z[++i]!='\0')x[i]=z[i];

  return x;
}

/* Compare 2 strings. */
int L_stcmp(const char* x, const char* y) {
  int i = -1;
  while (x[++i]==y[i]&&x[i]!='\0');
  return i == L_stln(x);
}
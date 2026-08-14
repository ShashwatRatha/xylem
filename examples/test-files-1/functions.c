#include "functions.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define FTABLE_INIT 8

void astFree(ASTNode *node);

FuncTable *funcTableInit() {
  FuncTable *fTable = calloc(1, sizeof(FuncTable));
  fTable->funcs = calloc(FTABLE_INIT, sizeof(Function));
  fTable->funcCount = 0;
  fTable->tableCap = FTABLE_INIT;

  return fTable;
}

char funcTableAdd(FuncTable *fTable, char *name, ASTNode *paramList,
                  ASTNode *funcBody) {
  if (fTable->funcCount >= fTable->tableCap) {
    fTable->tableCap <<= 1;
    Function *tmp = realloc(fTable->funcs, fTable->tableCap * sizeof(*tmp));
    if (!tmp) {
      return 1;
    }
    fTable->funcs = tmp;
  }

  fTable->funcs[fTable->funcCount] =
      (Function){ .name = name, .paramList = paramList, .funcBody = funcBody };

  fTable->funcCount++;
  return 0;
}

long int getFuncFromTable(const FuncTable *table, const char *funcName) {
  for (size_t i = 0; i < table->funcCount; i++) {
    if (!strcmp(table->funcs[i].name, funcName)) {
      return i;
    }
  }

  return -1;
}

void freeFuncTable(FuncTable *table) {
  for (size_t i = 0; i < table->funcCount; i++) {
    free(table->funcs[i].name);
    astFree(table->funcs[i].paramList);
    astFree(table->funcs[i].funcBody);
  }
  free(table->funcs);
  free(table);
}

#include "functions.h"
#include "runProgram.h"
#include "symtable.h"

int main(int argc, char *argv[]) {
  SymTable *sym = symInit(NULL);
  FuncTable *funcs = funcTableInit();
  if (argc > 1)
    runFile(argv[1], sym, funcs);
  else
    runREPL(sym, funcs);
  symFree(sym);
  freeFuncTable(funcs);
  return 0;
}

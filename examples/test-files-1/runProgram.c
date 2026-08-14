#include "runProgram.h"
#include "errors.h"
#include "eval.h"
#include "functions.h"
#include "lexer.h"
#include "parser.h"
#include "symtable.h"
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VERSION "3.0"
#define NEXT ">>> "
#define CONTINUE "... "
#define BANNER                                                                 \
  "xhu " VERSION "  —  integer expression interpreter\n\n"                   \
  "  arithmetic   :  + - * / %\n"                                              \
  "  bitwise      :  & | ^ ~ << >>\n"                                          \
  "  logical      :  && || !\n"                                                \
  "  comparison   :  == != < <= > >=\n"                                        \
  "  incr / decr  :  ++x  x++  --x  x--\n"                                     \
  "  assignment   :  = += -= *= /= %=  <<= >>=\n"                              \
  "  branching    :  if (cond) { } else { }\n"                                 \
  "  loops        :  while (cond) { }   for (init; cond; update) { }\n"        \
  "  loop control :  break   continue\n"                                       \
  "  functions    :  func name(a, b) { ... return expr }\n"                    \
  "  output       :  print expr\n"                                             \
  "  bindings     :  show\n"                                                   \
  "  exit         :  Ctrl-D\n\n"

void runLine(const char *line, SymTable *sym, FuncTable *funcs) {
  if (!line || line[0] == '\0')
    return;

  char *buf = strdup(line);
  if (!buf) {
    perror("strdup");
    return;
  }

  TokenArr toks = emitToks(buf);
  if (!toks.tokens) {
    free(buf);
    return;
  }

  Parser p;
  parserInit(&p, toks, sym);
  ASTNode *tree = parseProgram(&p);

  if (!p.had_error && tree) {
    Result res = evalAST(tree, sym, funcs);
    if (res.status == ERR)
      printError(res.errCode);
  }

  astFree(tree);
  freeTokens(toks.tokens);
  free(buf);
}

void runFile(const char *path, SymTable *sym, FuncTable *funcs) {
  FILE *f = fopen(path, "r");
  if (!f) {
    fprintf(stderr, "xhu: cannot open '%s': %s\n", path, strerror(errno));
    return;
  }

  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  rewind(f);

  char *buf = malloc(size + 1);
  if (!buf) {
    fclose(f);
    return;
  }

  size_t read = fread(buf, 1, (size_t)size, f);
  buf[read] = 0;
  fclose(f);

  runLine(buf, sym, funcs);
  free(buf);
}

void runREPL(SymTable *sym, FuncTable *funcs) {
  printf("%s", BANNER);
  char *buf = NULL;
  int depth = 0;
  size_t bufLen = 0;

  while (1) {
    printf("%s", depth <= 0 ? NEXT : CONTINUE);
    fflush(stdout);

    char *line = NULL;
    size_t cap = 0;
    ssize_t len = getline(&line, &cap, stdin);
    if (len == -1) {
      free(line);
      printf("\n");
      break;
    }
    if (line[len - 1] == '\n')
      line[--len] = 0;

    for (char *ptr = line; *ptr; ptr++) {
      if (*ptr == '{')
        depth++;
      else if (*ptr == '}')
        depth--;
    }

    char *tmp = realloc(buf, bufLen + len + 2);
    if (!tmp) {
      free(buf);
      free(line);
      return;
    }
    buf = tmp;
    memcpy(buf + bufLen, line, len);
    bufLen += len;
    buf[bufLen++] = '\n';
    buf[bufLen] = 0;
    free(line);

    if (depth <= 0) {
      depth = 0;
      runLine(buf, sym, funcs);
      free(buf);
      buf = NULL;
      bufLen = 0;
    }
  }

  free(buf);
}

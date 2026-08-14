#include "symtable.h"
#include "lexer.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// TODO: fix symbol table logic

SymTable *symInit(SymTable *parent) {
  SymTable *t = calloc(1, sizeof(*t));
  t->entries = calloc(SYM_INIT, sizeof(*t->entries));
  t->parent = parent;
  t->count = 0;
  t->capacity = SYM_INIT;

  return t;
}

SymResult symGet(SymTable *t, const char *name, int *out) {
  for (size_t i = 0; i < t->count; i++) {
    if (!strcmp(name, t->entries[i].name)) {
      *out = t->entries[i].val;
      return SYM_OK;
    }
  }
  if (t->parent)
    return symGet(t->parent, name, out);

  return SYM_ERR_UNDEF;
}

void symShow(const SymTable *t) {
  char **seen = NULL;
  size_t seenCount = 0;
  size_t seenCap = 0;

  printf("=== BINDINGS ===\n");

  const SymTable *scope = t;
  while (scope) {
    for (size_t i = 0; i < scope->count; i++) {
      const char *name = scope->entries[i].name;

      // skip if an inner scope already printed this name
      int shadowed = 0;
      for (size_t j = 0; j < seenCount; j++) {
        if (!strcmp(seen[j], name)) {
          shadowed = 1;
          break;
        }
      }
      if (shadowed)
        continue;

      printf("  %s = %d\n", name, scope->entries[i].val);

      // record name as seen
      if (seenCount >= seenCap) {
        seenCap = seenCap ? seenCap * 2 : 8;
        char **tmp = realloc(seen, seenCap * sizeof(*tmp));
        if (!tmp) {
          free(seen);
          return;
        }
        seen = tmp;
      }
      seen[seenCount++] = (char *)name;
    }
    scope = scope->parent;
  }

  free(seen);
  printf("\n");
}

void symFree(SymTable *t) {
  Symbol *entries = t->entries;
  for (size_t i = 0; i < t->count; i++)
    free(entries[i].name);
  free(t->entries);
  free(t);
}

static SymResult symAdd(SymTable *t, const char *name, int val) {
  if (t->count >= t->capacity) {
    t->capacity <<= 1;
    Symbol *tmp = realloc(t->entries, t->capacity * sizeof(*tmp));
    if (!tmp) {
      return SYM_MEM_ALLOC;
    }
    t->entries = tmp;
  }
  t->entries[t->count] = (Symbol){ .name = strdup(name), .val = val };
  t->count++;
  return SYM_OK;
}

static SymResult symUpdate(SymTable *t, int i, int op, int rval) {
  int val = t->entries[i].val;
  switch (op) {
  case ASSGN:
    val = rval;
    break;
  case ADD_ASSGN:
    val += rval;
    break;
  case SUB_ASSGN:
    val -= rval;
    break;
  case MUL_ASSGN:
    val *= rval;
    break;
  case DIV_ASSGN:
    if (rval == 0)
      return SYM_DIVZERO;
    val /= rval;
    break;
  case LSHIFT_ASSGN:
    val <<= rval;
    break;
  case RSHIFT_ASSGN:
    val >>= rval;
    break;
  default:
    return SYM_ERR_UNDEF;
  }
  t->entries[i].val = val;
  return SYM_OK;
}

/* symSet — create or update a binding.
 * Plain assignment (op == ASSGN) creates the variable if it does not exist.
 * Compound operators (+=, -= …) require the variable to already be bound. */

SymResult symSet(SymTable *t, const char *name, int val, int op) {
  if (op == ASSGN) {
    for (size_t i = 0; i < t->count; i++) {
      if (!strcmp(t->entries[i].name, name))
        return symUpdate(t, i, ASSGN, val);
    }
    return symAdd(t, name, val);
  }
  // Walk chain to find the table that owns the variable
  SymTable *owner = t;
  while (owner) {
    for (size_t i = 0; i < owner->count; i++) {
      if (!strcmp(name, owner->entries[i].name))
        return symUpdate(owner, i, op, val);
    }
    owner = owner->parent;
  }

  return SYM_ERR_UNDEF;
}

#include "lexer.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TOK_INIT 32
#define BUF_INIT 16

// State definitions
typedef enum {
  RST, // Reset: examining the current char to decide what comes next
  WTE, // Whitespace: skipping whitespace characters
  NUM, // Number:   accumulating digit characters into a number literal
  IDN, // Identifier: accumulating alphanumeric chars into an identifier name
  OPR  // Operator:  reading one or two-character operator sequences
} State;

// helper func for whitespace delims
static inline int isWhite(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

// helper func for operator-starting tokens
static inline int isOpStart(char c) {
  return c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c == '&' ||
         c == '|' || c == '^' || c == '<' || c == '>' || c == '=' || c == '!' ||
         c == '(' || c == ')' || c == '{' || c == '}' || c == '[' || c == ']' ||
         c == ',' || c == ';' || c == '~' || c == ':';
}

// helper func for digit chars
static inline int isDigit(char c) { return c >= '0' && c <= '9'; }

// helper to enforce correct identifier naming rules
static inline int isIdentStart(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}
static inline int isIdentChar(char c) { return isIdentStart(c) || isDigit(c); }

/* ─ Operator lookup table
 * greedily matches longest matching operator
 */

typedef struct
{
  const char *str;
  TokenType type;
} OpEntry;

static const OpEntry op_table[] = {
  /* 3-character operators first */
  { "<<=", LSHIFT_ASSGN },
  { ">>=", RSHIFT_ASSGN },

  /* 2-character operators */
  { "&&", AND },
  { "||", OR },
  { "<<", LSHIFT },
  { ">>", RSHIFT },
  { "<=", LET },
  { ">=", GET },
  { "==", EQ },
  { "!=", NEQ },
  { "++", INCR },
  { "--", DECR },
  { "+=", ADD_ASSGN },
  { "-=", SUB_ASSGN },
  { "*=", MUL_ASSGN },
  { "%=", MOD_ASSIGN },
  { "/=", DIV_ASSGN },

  /* 1-character operators */
  { "+", PLUS },
  { "-", MINUS },
  { "*", MUL },
  { "/", DIV },
  { "%", MOD },
  { "!", NOT },
  { "&", BIT_AND },
  { "|", BIT_OR },
  { "^", BIT_XOR },
  { "~", BIT_NOT },
  { "<", LT },
  { ">", GT },
  { "=", ASSGN },
  { "(", LPAREN },
  { ")", RPAREN },
  { "{", LBRACE },
  { "}", RBRACE },
  { "[", LBOX },
  { "]", RBOX },
  { ":", COLON },
  { ";", SEMICOLON },
  { ",", COMMA },
  { NULL, NULL_TYPE } /* sentinel */
};

// keywords
typedef struct
{
  const char *str;
  KeywordType kw;
} KwEntry;

static const KwEntry kw_table[] = {
  { "show", KW_SHOW },
  { "if", KW_IF },
  { "func", KW_FN },
  { "while", KW_WHILE },
  { "for", KW_FOR },
  { "else", KW_ELSE },
  { "print", KW_PRINT },
  { "break", KW_BREAK },
  { "continue", KW_CONTINUE },
  { "return", KW_RETURN },
  { NULL, 0 } /* sentinel */
};

/* checkAndAssignKeyword: given a null-terminated string, return 1 and set *out
 * if it matches a keyword, otherwise return 0.
 */
static int checkAndAssignKeyword(const char *str, KeywordType *out) {
  for (int i = 0; kw_table[i].str != NULL; i++) {
    if (strcmp(str, kw_table[i].str) == 0) {
      *out = kw_table[i].kw;
      return 1;
    }
  }
  return 0;
}

// helper for reporting errors
static void lexError(const char *msg, int col) {
  dprintf(STDERR_FILENO, "lex error at col %d: %s\n", col, msg);
}

// tokeniser func
TokenArr emitToks(char *str) {

  Token *tokens = malloc((TOK_INIT + 1) * sizeof(*tokens));
  if (!tokens) {
    dprintf(STDERR_FILENO, "lexer: token array alloc failed\n");
    return (TokenArr){ 0, NULL };
  }
  size_t tok_size = TOK_INIT;
  size_t next = 0; // next token to write
  char *curr = str;
  int col = 0;
  State st = RST;

  while (*curr) {

    // tokens array realloc
    if (next >= tok_size - 1) {
      tok_size *= 2;
      Token *tmp = realloc(tokens, (tok_size + 1) * sizeof(*tmp));
      if (!tmp) {
        free(tokens);
        return (TokenArr){ 0, NULL };
      }
      tokens = tmp;
    }

    switch (st) {
    // RST: dispatches state according to curr char
    case RST: {
      if (isWhite(*curr))
        st = WTE;
      else if (isDigit(*curr))
        st = NUM;
      else if (isIdentStart(*curr))
        st = IDN;
      else if (isOpStart(*curr))
        st = OPR;
      else {
        /* Unknown character — report and abort. */
        char msg[32];
        snprintf(msg, sizeof(msg), "unexpected char '%c'", *curr);
        lexError(msg, col);
        free(tokens);
        return (TokenArr){ 0, NULL };
      }
      break;
    }

    // WTE: consumes whitespace
    case WTE: {
      while (*curr && isWhite(*curr)) {
        curr++;
        col++;
      }
      st = RST;
      break;
    }

    // NUM: generates NUMBER tokens
    case NUM: {
      int val = 0;
      while (*curr && isDigit(*curr)) {
        val = val * 10 + (*curr - '0');
        curr++;
        col++;
      }
      if (*curr && isIdentChar(*curr)) {
        lexError("invalid number: letter follows digit", col);
        free(tokens);
        return (TokenArr){ 0, NULL };
      }
      tokens[next++] = (Token){ .type = NUMBER, .val = val };
      st = RST;
      break;
    }

    // IDN: deals with identifiers and keywords
    case IDN: {
      int buf_size = BUF_INIT;
      int idx = 0;
      char *buf = malloc((buf_size + 1) * sizeof(char));
      if (!buf) {
        free(tokens);
        return (TokenArr){ 0, NULL };
      }

      while (*curr && isIdentChar(*curr)) {
        if (idx >= buf_size) {
          buf_size += (buf_size >> 1); /* grow by 1.5x */
          char *tmp = realloc(buf, (buf_size + 1) * sizeof(char));
          if (!tmp) {
            free(buf);
            free(tokens);
            return (TokenArr){ 0, NULL };
          }
          buf = tmp;
        }
        buf[idx++] = *curr++;
        col++;
      }
      buf[idx] = '\0';

      KeywordType kw;
      if (checkAndAssignKeyword(buf, &kw)) {
        free(buf); /* keyword needs no string storage */
        tokens[next++] = (Token){ .type = KEYWORD, .kw = kw };
      } else {
        tokens[next++] = (Token){ .type = IDENT, .name = buf };
      }

      st = RST;
      break;
    }

    // OPR: deals with operators, matching greedily to the longest operator
    case OPR: {
      /* Peek up to 3 chars without yet advancing curr */
      char peek[4] = { 0 };
      for (int i = 0; i < 3 && curr[i]; i++)
        peek[i] = curr[i];

      int matched = 0;
      for (int i = 0; op_table[i].str != NULL; i++) {
        size_t len = strlen(op_table[i].str);
        if (strncmp(peek, op_table[i].str, len) == 0) {
          tokens[next++] = (Token){ .type = op_table[i].type, .name = NULL };
          curr += len;
          col += len;
          matched = 1;
          break;
        }
      }
      if (!matched) {
        // guard against unknown user-entered operator
        char msg[32];
        snprintf(msg, sizeof(msg), "unknown operator '%c'", *curr);
        lexError(msg, col);
        free(tokens);
        return (TokenArr){ 0, NULL };
      }
      st = RST;
      break;
    }

    } // end switch
  } // end while

  if (next >= tok_size - 1) {
    Token *tmp = realloc(tokens, (next + 2) * sizeof(*tmp));
    if (!tmp) {
      free(tokens);
      return (TokenArr){ 0, NULL };
    }
    tokens = tmp;
  }
  tokens[next++] = (Token){ .type = NULL_TYPE, .name = NULL };

  return (TokenArr){ .count = next, .tokens = tokens };
}

// only IDENT tokens' names need to be freed as they are allocated on the heap
void freeTokens(Token *toks) {
  if (!toks)
    return;
  for (size_t i = 0; toks[i].type != NULL_TYPE; i++) {
    if (toks[i].type == IDENT) {
      free(toks[i].name);
      toks[i].name = NULL;
    }
  }
  free(toks);
}

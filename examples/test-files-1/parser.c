#include "parser.h"
#include "lexer.h"
#include <stddef.h>
#include <stdlib.h>

#define STMTS_INIT 8

// <assign_op>  ::= "="  | "+=" | "-=" | "*=" | "/=" | "%="
//                | "<<=" | ">>="

void parserInit(Parser *p, TokenArr toks, SymTable *sym) {
  p->toks = toks;
  p->sym = sym;
  p->pos = 0;
  p->had_error = 0;
}

// <program>    ::= <stmts>
// <block>      ::= "{" <stmts> "}"

ASTNode *parseProgram(Parser *p) { return astProgram(parseStmts(p)); }

ASTNode *parseBlock(Parser *p) {
  if (parserConsume(p).type != LBRACE) {
    p->had_error = 1;
    return NULL;
  }

  ASTNode *block = parseStmts(p);
  if (!block) {
    p->had_error = 1;
    return NULL;
  }

  if (parserConsume(p).type != RBRACE) {
    astFree(block);
    p->had_error = 1;
    return NULL;
  }

  return astBlock(block);
}

// <stmts>      ::= <stmt> <stmts> | ε

ASTNode *parseStmts(Parser *p) {
  ASTNode **stmts = malloc(STMTS_INIT * sizeof(*stmts));
  if (!stmts)
    return NULL;

  size_t stmtIdx = 0, stmtCap = STMTS_INIT;

  while (parserPeek(p) != NULL_TYPE && parserPeek(p) != RBRACE) {
    if (p->had_error)
      break;
    ASTNode *stmt = parseStmt(p);
    if (!stmt)
      break;

    if (stmtIdx >= stmtCap) {
      stmtCap <<= 1;
      ASTNode **tmp = realloc(stmts, stmtCap * sizeof(*stmts));
      if (!tmp) {
        for (size_t i = 0; i < stmtIdx; i++)
          astFree(stmts[i]);
        free(stmts);
        return NULL;
      }
      stmts = tmp;
    }

    stmts[stmtIdx++] = stmt;
  }

  return astStmts(stmts, stmtIdx);
}

// <stmt>      ::= <func_decl>
//               | <if_stmt>
//               | <while_stmt>
//               | <for_stmt>
//               | "break"
//               | "continue"
//               | "return" [ <expr> ]
//               | "print"  <expr>
//               | "show"
//               | IDENT <assign_op> <expr>
//               | <expr>

ASTNode *parseStmt(Parser *p) {
  switch (parserPeek(p)) {
  case KEYWORD: {
    KeywordType kw = parserConsume(p).kw;
    switch (kw) {
    case KW_SHOW:
      return astShow();
    case KW_RETURN:
      return astReturn(parseExpr(p));
    case KW_BREAK:
      return astBreak();
    case KW_CONTINUE:
      return astContinue();
    case KW_PRINT:
      return astPrint(parseExpr(p));
    case KW_IF:
      return parseIf(p);
    case KW_WHILE:
      return parseWhile(p);
    case KW_FOR:
      return parseFor(p);
    case KW_FN:
      return parseFunction(p);
    default:
      p->had_error = 1;
      return NULL;
    }
    break;
  }
  case IDENT: {
    if (isAssgnOp(parserPeek2(p))) {
      char *name = parserConsume(p).name;
      TokenType op = parserConsume(p).type;
      ASTNode *expr = parseExpr(p);

      return astAssgn(op, name, expr);
    } else {
      return astExprStmt(parseExpr(p));
    }
  }
  default: {
    return astExprStmt(parseExpr(p));
  }
  }
}

// <func_decl>  ::= "func" IDENT "(" <param_list> ")" <block>

ASTNode *parseFunction(Parser *p) {
  if (parserPeek(p) != IDENT) {
    p->had_error = 1;
    return NULL;
  }

  char *funcName = parserConsume(p).name;
  if (parserConsume(p).type != LPAREN) {
    p->had_error = 1;
    return NULL;
  }

  ASTNode *paramList = parseParams(p);
  if (!paramList) {
    p->had_error = 1;
    return NULL;
  }

  if (parserConsume(p).type != RPAREN) {
    astFree(paramList);
    p->had_error = 1;
    return NULL;
  }

  ASTNode *body = parseBlock(p);
  if (!body) {
    astFree(paramList);
    p->had_error = 1;
    return NULL;
  }

  return astFunction(funcName, paramList, body);
}

// <param_list> ::= ε
//                | IDENT { "," IDENT }

#define PARAMS_INIT 4

ASTNode *parseParams(Parser *p) {
  ASTNode **params = NULL;
  size_t paramIdx = 0, paramCap = 0;

  if (parserPeek(p) == IDENT) {
    params = calloc(PARAMS_INIT, sizeof(*params));
    if (!params) {
      p->had_error = 1;
      return NULL;
    }
    paramCap = PARAMS_INIT;
  }

  while (parserPeek(p) == IDENT) {
    if (paramIdx >= paramCap) {
      paramCap <<= 1;
      ASTNode **tmp = realloc(params, paramCap * sizeof(*params));
      if (!tmp) {
        for (size_t i = 0; i < paramIdx; i++) {
          astFree(params[i]);
        }
        free(params);
        p->had_error = 1;
        return NULL;
      }
      params = tmp;
    }
    ASTNode *param = astVar(parserConsume(p).name);
    params[paramIdx++] = param;
    if (parserPeek(p) != COMMA)
      break;
    parserConsume(p);
  }

  return astStmts(params, paramIdx);
}

// <if_stmt>    ::= "if"   "(" <expr> ")" <block>
// [ "else" <block> ]

ASTNode *parseIf(Parser *p) {
  if (parserConsume(p).type != LPAREN) {
    p->had_error = 1;
    return NULL;
  }
  ASTNode *condition = parseExpr(p);
  if (!condition) {
    p->had_error = 1;
    return NULL;
  }
  if (parserConsume(p).type != RPAREN) {
    astFree(condition);
    p->had_error = 1;
    return NULL;
  }
  ASTNode *thenBlock = parseBlock(p);
  if (!thenBlock) {
    astFree(condition);
    p->had_error = 1;
    return NULL;
  }
  ASTNode *elseBlock = NULL;

  Token t = parserCurr(p);
  if (t.type == KEYWORD && t.kw == KW_ELSE) {
    parserConsume(p);
    elseBlock = parseBlock(p);
    if (!elseBlock) {
      astFree(condition);
      astFree(thenBlock);
      p->had_error = 1;
      return NULL;
    }
  }
  return astIf(condition, thenBlock, elseBlock);
}

// <while_stmt> ::= "while" "(" <expr> ")" <block>

ASTNode *parseWhile(Parser *p) {
  if (parserConsume(p).type != LPAREN) {
    p->had_error = 1;
    return NULL;
  }

  ASTNode *condition = parseExpr(p);
  if (!condition) {
    p->had_error = 1;
    return NULL;
  }

  if (parserConsume(p).type != RPAREN) {
    astFree(condition);
    p->had_error = 1;
    return NULL;
  }

  ASTNode *body = parseBlock(p);
  if (!body) {
    astFree(condition);
    p->had_error = 1;
    return NULL;
  }

  return astWhile(condition, body);
}

// <for_stmt>   ::= "for" "(" <for_clause> ";" <expr> ";" <for_clause> ")"
// <block>

ASTNode *parseFor(Parser *p) {
  if (parserConsume(p).type != LPAREN) {
    p->had_error = 1;
    return NULL;
  }

  ASTNode *initiation = parseForClause(p);
  if (!initiation) {
    p->had_error = 1;
    return NULL;
  }

  if (parserConsume(p).type != SEMICOLON) {
    astFree(initiation);
    p->had_error = 1;
    return NULL;
  }

  ASTNode *condition = parseExpr(p);
  if (!condition) {
    astFree(initiation);
    p->had_error = 1;
    return NULL;
  }

  if (parserConsume(p).type != SEMICOLON) {
    astFree(initiation);
    astFree(condition);
    p->had_error = 1;
    return NULL;
  }

  ASTNode *updateClause = parseForClause(p);
  if (!updateClause) {
    astFree(initiation);
    astFree(condition);
    p->had_error = 1;
    return NULL;
  }

  if (parserConsume(p).type != RPAREN) {
    astFree(initiation);
    astFree(condition);
    astFree(updateClause);
    p->had_error = 1;
    return NULL;
  }

  ASTNode *body = parseBlock(p);
  if (!body) {
    astFree(initiation);
    astFree(condition);
    astFree(updateClause);
    p->had_error = 1;
    return NULL;
  }

  return astFor(initiation, condition, updateClause, body);
}

// <for_clause> ::= ε
//                | IDENT <assign_op> <expr>
//                | <expr>

ASTNode *parseForClause(Parser *p) {
  if (parserPeek(p) == IDENT) {
    if (isAssgnOp(parserPeek2(p))) {
      char *name = parserConsume(p).name;
      TokenType op = parserConsume(p).type;
      ASTNode *rval = parseExpr(p);
      return astAssgn(op, name, rval);
    }
  }

  return parseExpr(p);
}

// <expr>       ::= <logic_or>

ASTNode *parseExpr(Parser *p) { return parseLogicOr(p); }

// <logic_or>   ::= <logic_and>
//                | <logic_and> "||" <logic_or>

ASTNode *parseLogicOr(Parser *p) {
  ASTNode *left = parseLogicAnd(p);
  if (parserPeek(p) == OR) {
    parserConsume(p);
    return astBinOp(OR, left, parseLogicOr(p));
  }

  return left;
}

// <logic_and>  ::= <bit_or>
//                | <bit_or> "&&" <logic_and>

ASTNode *parseLogicAnd(Parser *p) {
  ASTNode *left = parseBitOr(p);
  if (parserPeek(p) == AND) {
    parserConsume(p);
    return astBinOp(AND, left, parseLogicAnd(p));
  }

  return left;
}

// <bit_or>     ::= <bit_xor>
//                | <bit_xor> "|" <bit_or>

ASTNode *parseBitOr(Parser *p) {
  ASTNode *left = parseBitXor(p);
  if (parserPeek(p) == BIT_OR) {
    parserConsume(p);
    return astBinOp(BIT_OR, left, parseBitOr(p));
  }

  return left;
}

// <bit_xor>    ::= <bit_and>
//                | <bit_and> "^" <bit_xor>

ASTNode *parseBitXor(Parser *p) {
  ASTNode *left = parseBitAnd(p);
  if (parserPeek(p) == BIT_XOR) {
    parserConsume(p);
    return astBinOp(BIT_XOR, left, parseBitXor(p));
  }

  return left;
}

// <bit_and>    ::= <equality>
//                | <equality> "&" <bit_and>

ASTNode *parseBitAnd(Parser *p) {
  ASTNode *left = parseEquality(p);
  if (parserPeek(p) == BIT_AND) {
    parserConsume(p);
    return astBinOp(BIT_AND, left, parseBitAnd(p));
  }

  return left;
}

// <equality>   ::= <relational>
//                | <equality> "==" <relational>
//                | <equality> "!=" <relational>

ASTNode *parseEquality(Parser *p) {
  ASTNode *left = parseRelational(p);
  while (parserPeek(p) == EQ || parserPeek(p) == NEQ) {
    TokenType op = parserConsume(p).type;
    left = astBinOp(op, left, parseRelational(p));
  }

  return left;
}

// <relational> ::= <shift>
//                | <relational> "<"  <shift>
//                | <relational> "<=" <shift>
//                | <relational> ">"  <shift>
//                | <relational> ">=" <shift>

ASTNode *parseRelational(Parser *p) {
  ASTNode *left = parseShift(p);
  while (parserPeek(p) == LT || parserPeek(p) == GT || parserPeek(p) == LET ||
         parserPeek(p) == GET) {
    TokenType op = parserConsume(p).type;
    left = astBinOp(op, left, parseShift(p));
  }

  return left;
}

// <shift>      ::= <additive>
//                | <shift> "<<" <additive>
//                | <shift> ">>" <additive>

ASTNode *parseShift(Parser *p) {
  ASTNode *left = parseAdditive(p);
  while (parserPeek(p) == LSHIFT || parserPeek(p) == RSHIFT) {
    TokenType op = parserConsume(p).type;
    left = astBinOp(op, left, parseAdditive(p));
  }

  return left;
}

// <additive>   ::= <term>
//                | <additive> "+" <term>
//                | <additive> "-" <term>

ASTNode *parseAdditive(Parser *p) {
  ASTNode *left = parseTerm(p);
  while (parserPeek(p) == PLUS || parserPeek(p) == MINUS) {
    TokenType op = parserConsume(p).type;
    ASTNode *right = parseTerm(p);
    left = astBinOp(op, left, right);
  }

  return left;
}

// <term>       ::= <unary>
//                | <term> "*" <unary>
//                | <term> "/" <unary>
//                | <term> "%" <unary>

ASTNode *parseTerm(Parser *p) {
  ASTNode *left = parseUnary(p);
  while (parserPeek(p) == MUL || parserPeek(p) == DIV || parserPeek(p) == MOD) {
    TokenType op = parserConsume(p).type;
    ASTNode *right = parseUnary(p);
    left = astBinOp(op, left, right);
  }

  return left;
}

// <unary>      ::= <postfix>
//                | "-"  <unary>
//                | "!"  <unary>
//                | "~"  <unary>
//                | "++" <unary>
//                | "--" <unary>

ASTNode *parseUnary(Parser *p) {
  TokenType tp = parserPeek(p);
  if (tp == MINUS || tp == INCR || tp == DECR || tp == NOT || tp == BIT_NOT) {
    TokenType op = parserConsume(p).type;

    ASTNode *operand = parseUnary(p);
    if (!operand || ((op == INCR || op == DECR) && operand->type != nodeVar)) {
      astFree(operand);
      p->had_error = 1;
      return NULL;
    }

    if (op == MINUS || op == NOT || op == BIT_NOT)
      return astUnary(op, operand);
    else {
      ASTNode *returnVal =
          (op == INCR) ? astPreIncr(operand->name) : astPreDecr(operand->name);
      astFree(operand);
      return returnVal;
    }
  } else
    return parsePostfix(p);
}

// <postfix>    ::= <primary>
//                | <primary> "++"
//                | <primary> "--"

ASTNode *parsePostfix(Parser *p) {
  ASTNode *primary = parsePrimary(p);
  TokenType op = parserPeek(p);

  if (op == INCR || op == DECR) {
    if (!primary || primary->type != nodeVar) {
      astFree(primary);
      p->had_error = 1;
      return NULL;
    }
    parserConsume(p);
    ASTNode *returnVal =
        (op == INCR) ? astPostIncr(primary->name) : astPostDecr(primary->name);
    astFree(primary);
    return returnVal;
  } else
    return primary;
}

// <primary>    ::= NUMBER
//                | IDENT
//                | IDENT "(" <arg_list> ")"
//                | "(" <expr> ")"
// NUMBER       ::= DIGIT { DIGIT }
// IDENT        ::= IDENT_START { IDENT_CHAR }
// IDENT_START  ::= [a-zA-Z_]
// IDENT_CHAR   ::= [a-zA-Z0-9_]
// DIGIT        ::= [0-9]
// COMMENT      ::= ";" { any char except newline }

ASTNode *parsePrimary(Parser *p) {
  TokenType tp = parserPeek(p);
  if (tp == LPAREN) {
    parserConsume(p);
    ASTNode *expr = parseExpr(p);
    if (!expr) {
      p->had_error = 1;
      return NULL;
    }
    if (parserConsume(p).type != RPAREN) {
      astFree(expr);
      p->had_error = 1;
      return NULL;
    }
    return expr;
  } else if (tp == NUMBER) {
    return astNum(parserConsume(p).val);
  } else if (tp == IDENT) {
    char *name = parserConsume(p).name;
    if (parserPeek(p) == LPAREN) {
      parserConsume(p);
      ASTNode *argList = parseArgList(p);
      if (!argList) {
        p->had_error = 1;
        return NULL;
      }
      if (parserConsume(p).type != RPAREN) {
        astFree(argList);
        p->had_error = 1;
        return NULL;
      }

      return astFuncCall(name, argList);
    }
    return astVar(name);
  }

  p->had_error = 1;
  return NULL;
}

// <arg_list>   ::= ε
//                | <expr> { "," <expr> }

#define ARGS_INIT 4

ASTNode *parseArgList(Parser *p) {
  size_t argIdx = 0, argCap = ARGS_INIT;

  ASTNode **args = calloc(ARGS_INIT, sizeof(*args));
  if (!args) {
    p->had_error = 1;
    return NULL;
  }

  while (parserPeek(p) != RPAREN) {
    if (argIdx >= argCap) {
      argCap <<= 1;
      ASTNode **tmp = realloc(args, argCap * sizeof(*args));
      if (!tmp) {
        for (size_t i = 0; i < argIdx; i++) {
          astFree(args[i]);
        }
        free(args);
        p->had_error = 1;
        return NULL;
      }
      args = tmp;
    }
    ASTNode *arg = parseExpr(p);
    args[argIdx++] = arg;
    if (parserPeek(p) != COMMA)
      break;
    parserConsume(p);
  }

  if (argIdx == 0) {
    free(args);
    args = NULL;
  }

  return astStmts(args, argIdx);
}

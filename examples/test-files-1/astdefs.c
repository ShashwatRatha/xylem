#include "lexer.h"
#include "parser.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// default constructor for AST node
static ASTNode *astInit(void) {
  ASTNode *node = calloc(1, sizeof(ASTNode));
  return node;
}

// number node
ASTNode *astNum(int val) {
  ASTNode *node = astInit();

  *node = (ASTNode){ .type = nodeNum, .val = val };

  return node;
}

// variable node
ASTNode *astVar(const char *name) {
  ASTNode *node = astInit();

  *node = (ASTNode){ .type = nodeVar, .name = strdup(name) };

  return node;
}

// binary operation node
ASTNode *astBinOp(TokenType tp, ASTNode *left, ASTNode *right) {
  ASTNode *node = astInit();

  *node = (ASTNode){ .type = nodeBinOp,
                     .binOp = { .op = tp, .left = left, .right = right } };

  return node;
}

// unary operation node
ASTNode *astUnary(TokenType op, ASTNode *operand) {
  ASTNode *node = astInit();

  *node = (ASTNode){ .type = nodeUnary,
                     .standaloneNode = { .op = op, .operand = operand } };

  return node;
}

// pre-increment opearator node
ASTNode *astPreIncr(const char *name) {
  ASTNode *node = astInit();

  *node = (ASTNode){ .type = nodePreInc, .name = strdup(name) };

  return node;
}

// pre-decrement operator node
ASTNode *astPreDecr(const char *name) {
  ASTNode *node = astInit();

  *node = (ASTNode){ .type = nodePreDec, .name = strdup(name) };

  return node;
}

// post-increment operator node
ASTNode *astPostIncr(const char *name) {
  ASTNode *node = astInit();

  *node = (ASTNode){ .type = nodePostInc, .name = strdup(name) };

  return node;
}

// post-decrement operator node
ASTNode *astPostDecr(const char *name) {
  ASTNode *node = astInit();

  *node = (ASTNode){ .type = nodePostDec, .name = strdup(name) };

  return node;
}

// assignment statement node
ASTNode *astAssgn(TokenType op, const char *name, ASTNode *operand) {
  ASTNode *node = astInit();

  *node = (ASTNode){ .type = nodeAssign,
                     .assgnNode = {
                         .op = op, .name = strdup(name), .rvalue = operand } };

  return node;
}

// node for show keyword
ASTNode *astShow(void) {
  ASTNode *node = astInit();

  *node = (ASTNode){ .type = nodeShow };

  return node;
}

// expression node
ASTNode *astExprStmt(ASTNode *expr) {
  ASTNode *node = astInit();

  *node =
      (ASTNode){ .type = nodeExprStmt, .standaloneNode = { .operand = expr } };

  return node;
}

// complete program node
ASTNode *astStmts(ASTNode **stmts, size_t count) {
  ASTNode *node = astInit();

  *node = (ASTNode){ .type = nodeStmts,
                     .block = { .stmts = stmts, .stmtCount = count } };

  return node;
}

ASTNode *astReturn(ASTNode *expr) {
  ASTNode *node = astInit();

  *node =
      (ASTNode){ .type = nodeReturn, .standaloneNode = { .operand = expr } };

  return node;
}

ASTNode *astBreak() {
  ASTNode *node = astInit();

  *node = (ASTNode){ .type = nodeBreak };

  return node;
}

ASTNode *astContinue() {
  ASTNode *node = astInit();

  *node = (ASTNode){ .type = nodeContinue };

  return node;
}

ASTNode *astIf(ASTNode *condition, ASTNode *thenBlock, ASTNode *elseBlock) {
  ASTNode *node = astInit();

  *node = (ASTNode){ .type = nodeIf,
                     .ifNode = { .condition = condition,
                                 .thenBlock = thenBlock,
                                 .elseBlock = elseBlock } };

  return node;
}

ASTNode *astPrint(ASTNode *expression) {
  ASTNode *node = astInit();

  *node = (ASTNode){ .type = nodePrint,
                     .standaloneNode = { .operand = expression } };

  return node;
}

/*
ASTNode* astFor(ASTNode* init, ASTNode* cond, ASTNode* update) {
  ASTNode* node = astInit();

  node->type = nodeFor;
  // for i:[0,121] {}
  // "for" IDENT ":" "[" <start> "," <end> "]" <block>
}*/

ASTNode *astWhile(ASTNode *condition, ASTNode *body) {
  ASTNode *node = astInit();

  *node = (ASTNode){ .type = nodeWhile,
                     .whileNode = { .condition = condition, .body = body } };

  return node;
}

ASTNode *astFunction(char *funcName, ASTNode *paramList, ASTNode *body) {
  ASTNode *node = astInit();

  *node = (ASTNode){ .type = nodeFn,
                     .funcNode = { .name = strdup(funcName),
                                   .paramList = paramList,
                                   .body = body } };

  return node;
}

ASTNode *astFuncCall(char *funcName, ASTNode *argList) {
  ASTNode *node = astInit();

  *node =
      (ASTNode){ .type = nodeFnCall,
                 .funcCall = { .name = strdup(funcName), .argList = argList } };

  return node;
}

ASTNode *astFor(ASTNode *init, ASTNode *condition, ASTNode *update,
                ASTNode *body) {
  ASTNode *node = astInit();

  *node = (ASTNode){ .type = nodeFor,
                     .forNode = { .init = init,
                                  .cond = condition,
                                  .update = update,
                                  .body = body } };

  return node;
}

ASTNode *astProgram(ASTNode *block) {
  ASTNode *node = astInit();

  *node =
      (ASTNode){ .type = nodeProgram, .standaloneNode = { .operand = block } };

  return node;
}

ASTNode *astBlock(ASTNode *block) {
  ASTNode *node = astInit();

  *node =
      (ASTNode){ .type = nodeBlock, .standaloneNode = { .operand = block } };

  return node;
}

void astFree(ASTNode *node) {
  if (!node)
    return;

  switch (node->type) {
  case nodePreInc:
  case nodePreDec:
  case nodePostInc:
  case nodePostDec:
  case nodeVar:
    free(node->name);
    break;
  case nodeIf:
    astFree(node->ifNode.condition);
    astFree(node->ifNode.thenBlock);
    astFree(node->ifNode.elseBlock);
    break;
  case nodeWhile:
    astFree(node->whileNode.condition);
    astFree(node->whileNode.body);
    break;
  case nodeBinOp:
    astFree(node->binOp.left);
    astFree(node->binOp.right);
    break;
  case nodeAssign:
    free(node->assgnNode.name);
    astFree(node->assgnNode.rvalue);
    break;
  case nodeStmts:
    for (size_t i = 0; i < node->block.stmtCount; i++) {
      astFree(node->block.stmts[i]);
    }
    free(node->block.stmts);
    break;
  case nodeFn:
    free(node->funcNode.name);
    astFree(node->funcNode.body);
    astFree(node->funcNode.paramList);
    break;
  case nodeFor:
    astFree(node->forNode.body);
    astFree(node->forNode.init);
    astFree(node->forNode.cond);
    astFree(node->forNode.update);
    break;
  case nodeFnCall:
    free(node->funcCall.name);
    astFree(node->funcCall.argList);
    break;
  case nodeExprStmt:
  case nodeUnary:
  case nodeReturn:
  case nodeProgram:
  case nodeBlock:
  case nodePrint:
    astFree(node->standaloneNode.operand);
    break;
  default:
    break;
  }

  free(node);
}

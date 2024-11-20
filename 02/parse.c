#include <stdbool.h>
#include <stdio.h>

#include "lpp.h"

/**
 * @brief 段付の深さを表す変数
 * 初期値は0であり、段付が深くなるごとにインクリメントされる
 */
static int indent_level = 0;
/**
 * @brief 行頭が改行されるべきかを文脈に併せて判断する変数。
 * トークンのリストにも改行されて段落の一番最初にあるかどうかを表すフラグがあるが、
 * この変数は文脈に応じて改行されるべきかを判断するための変数である。
 * 二つのフラグの論理和で改行されるべきかを判断する。
 */
static bool at_bol = false;

static bool has_space = false;

/**
 * @brief 現在のトークンを指すグローバルな変数
 * 
 */
static Token * cur;
/**
 * @brief 繰り返し文の深さを表す変数
 * 
 */
static int iteration_level = 0;
static const char * token_str[NUMOFTOKEN + 1] = {
  "",       "NAME",   "program",   "var",     "array",   "of",     "begin",   "end",  "if",
  "then",   "else",   "procedure", "return",  "call",    "while",  "do",      "not",  "or",
  "div",    "and",    "char",      "integer", "boolean", "readln", "writeln", "true", "false",
  "NUMBER", "STRING", "+",         "-",       "*",       "=",      "<>",      "<",    "<=",
  ">",      ">=",     "(",         ")",       "[",       "]",      ":=",      ".",    ",",
  ":",      ";",      "read",      "write",   "break"};

static void printIndent();
static void printToken(const Token *);
static void consumeToken(Token *);
static int parseType();
static int parseVarNames();
static int parseVarDeclaration();
static int parseTerm();
static int parseSimpleExpression();
static int parseExpression();
static int parseFactor();
static int parseStatement();
static int parseCompoundStatement();
static int parseFormalParamters();
static int parseSubProgram();
static int parseBlock();
static int parseProgram();
static int parseAssignment();
static int parseCondition();
static int parseIteration();
static int parseCall();
static int parseVar();
static int parseInput();
static int parseOutputFormat();
static int parseOutputStatement();

static void printIndent()
{
  for (int i = 0; i < indent_level; i++) {
    printf("    ");
  }
}

static void printToken(const Token * tok)
{
  if (tok->has_space || has_space) printf(" ");
  if (tok->at_bol || at_bol) {
    printf("\n");
    printIndent();
  }

  if (tok->kind == TK_KEYWORD || tok->kind == TK_PUNCT) {
    printf("%s", token_str[cur->id]);
  } else if (cur->kind == TK_NUM) {
    printf("%d", cur->num);
  } else if (cur->kind == TK_STR) {
    printf("'%s'", cur->str);
  } else {
    printf("%s", cur->str);
  }
  fflush(stdout);
  at_bol = false;
  has_space = false;
}

static void consumeToken(Token * tok)
{
  printToken(tok);
  cur = cur->next;
}

static bool isparseRelOp()
{
  switch (cur->id) {
    case TEQUAL:
    case TNOTEQ:
    case TLE:
    case TLEEQ:
    case TGR:
    case TGREQ:
      return true;
    default:
      return false;
  }
}

static bool isMulOp()
{
  switch (cur->id) {
    case TSTAR:
    case TDIV:
    case TAND:
      consumeToken(cur);
      return true;
    default:
      return false;
  }
}

static bool isAddOp()
{
  switch (cur->id) {
    case TPLUS:
    case TMINUS:
    case TOR:
      return true;
    default:
      return false;
  }
}

static bool isStdType()
{
  switch (cur->id) {
    case TINTEGER:
    case TBOOLEAN:
    case TCHAR:
      return true;
    default:
      return false;
  }
}

static int parseType()
{
  if (isStdType()) {
    consumeToken(cur);
  } else if (cur->id == TARRAY) {
    consumeToken(cur);
    if (cur->id != TLSQPAREN) return error("\nError at %d: Expected '['", cur->line_no);
    consumeToken(cur);
    if (cur->id != TNUMBER) return error("\nError at %d: Expected number", cur->line_no);
    consumeToken(cur);
    if (cur->id != TRSQPAREN) return error("\nError at %d: Expected ']'", cur->line_no);
    consumeToken(cur);
    if (cur->id != TOF) return error("\nError at %d: Expected 'of'", cur->line_no);
    consumeToken(cur);
    if (isStdType()) return error("\nError at %d: Expected type", cur->line_no);
    consumeToken(cur);
  } else {
    return error("\nError at %d: Expected type", cur->line_no);
  }
  return NORMAL;
}

static int parseVarNames()
{
  if (cur->id != TNAME) return ERROR;
  consumeToken(cur);
  while (cur->id == TCOMMA) {
    consumeToken(cur);
    if (cur->id != TNAME) return error("\nError at %d: Expected variable name", cur->line_no);
    consumeToken(cur);
  }

  return NORMAL;
}

static int parseVarDeclaration()
{
  if (cur->id != TVAR) return error("\nError at %d: Expected 'var'", cur->line_no);
  indent_level++;
  consumeToken(cur);
  indent_level++;
  at_bol = true;
  if (parseVarNames() == ERROR) return error("\nError at %d: Expected variable name", cur->line_no);

  if (cur->id != TCOLON) return error("\nError at %d: Expected ':'", cur->line_no);
  consumeToken(cur);

  if (parseType() == ERROR) return ERROR;

  if (cur->id != TSEMI) return error("\nError at %d: Expected ';'", cur->line_no);
  consumeToken(cur);

  while (parseVarNames() == NORMAL) {
    if (cur->id != TCOLON) return error("\nError at %d: Expected ':'", cur->line_no);
    consumeToken(cur);

    if (parseType() == ERROR) return ERROR;

    if (cur->id != TSEMI) return error("\nError at %d: Expected ';'", cur->line_no);
    consumeToken(cur);
  }
  indent_level -= 2;

  return NORMAL;
}

static int parseTerm()
{
  if (parseFactor() == ERROR) return ERROR;
  while (isMulOp()) {
    consumeToken(cur);
    if (parseFactor() == ERROR) return ERROR;
  }
  return NORMAL;
}

static int parseSimpleExpression()
{
  if (cur->id == TPLUS || cur->id == TMINUS) consumeToken(cur);

  if (parseTerm() == ERROR) return ERROR;

  while (isAddOp()) {
    consumeToken(cur);
    if (parseTerm() == ERROR) return ERROR;
  }
  return NORMAL;
}

static int parseExpression()
{
  if (parseSimpleExpression() == ERROR) return ERROR;
  while (isparseRelOp()) {
    consumeToken(cur);

    if (parseSimpleExpression() == ERROR) return ERROR;
  }
  return NORMAL;
}

static int parseFactor()
{
  switch (cur->id) {
    // 変数
    case TNAME:
      if (parseVar() == ERROR) return ERROR;
      break;
    // 定数
    case TNUMBER:
    case TFALSE:
    case TTRUE:
    case TSTRING:
      consumeToken(cur);
      break;
    case TLPAREN:
      consumeToken(cur);
      has_space = true;
      if (parseExpression() == ERROR) return ERROR;
      if (cur->id != TRPAREN) return error("\nError at %d: Expected ')'", cur->line_no);
      consumeToken(cur);
      break;
    case TNOT:
      consumeToken(cur);
      if (parseFactor() == ERROR) return ERROR;
      break;
    //
    case TINTEGER:
    case TBOOLEAN:
    case TCHAR:
      consumeToken(cur);
      if (cur->id != TLPAREN) error("\nError at %d: Expected '('", cur->line_no);
      consumeToken(cur);
      if (parseExpression() == ERROR) return ERROR;
      if (cur->id != TRPAREN) error("\nError at %d: Expected ')'", cur->line_no);
      consumeToken(cur);
      break;
    default:
      // return error("\nError at %d: Expected factor", cur->line_no);
      break;
  }
  return NORMAL;
}

static int parseAssignment()
{
  if (parseVar() == ERROR) return ERROR;

  if (cur->id != TASSIGN) return error("\nError at %d: Expected ':='", cur->line_no);
  consumeToken(cur);

  if (parseExpression() == ERROR) return ERROR;
  return NORMAL;
}

static int parseCondition()
{
  if (cur->id != TIF) return error("\nError at %d: Expected 'if'", cur->line_no);
  consumeToken(cur);
  if (parseExpression() == ERROR) return ERROR;
  if (cur->id != TTHEN) return error("\nError at %d: Expected 'then'", cur->line_no);
  consumeToken(cur);
  at_bol = true;
  indent_level++;
  if (parseStatement() == ERROR) return ERROR;
  if (cur->id == TELSE) {
    indent_level--;
    consumeToken(cur);
    at_bol = true;
    if (cur->id != TBEGIN) indent_level++;

    if (parseStatement() == ERROR) return ERROR;
    indent_level--;
  }
  return NORMAL;
}

static int parseIteration()
{
  if (cur->id != TWHILE) return error("\nError at %d: Expected 'while'", cur->line_no);
  consumeToken(cur);
  if (parseExpression() == ERROR) return ERROR;
  if (cur->id != TDO) return error("\nError at %d: Expected 'do'", cur->line_no);
  consumeToken(cur);
  iteration_level++;
  if (parseStatement() == ERROR) return ERROR;
  iteration_level--;
  return NORMAL;
}

static int parseCall()
{
  if (cur->id != TCALL) return error("\nError at %d: Expected 'call'", cur->line_no);
  consumeToken(cur);
  if (cur->id != TNAME) return error("\nError at %d: Expected procedure name", cur->line_no);
  consumeToken(cur);
  if (cur->id == TLPAREN) {
    consumeToken(cur);
    // 式の並び
    if (parseExpression() == ERROR) return ERROR;
    while (cur->id == TCOMMA) {
      consumeToken(cur);
      if (parseExpression() == ERROR) return ERROR;
    }
    if (cur->id != TRPAREN) return error("\nError at %d: Expected ')'", cur->line_no);
    consumeToken(cur);
  }
  return NORMAL;
}

static int parseVar()
{
  if (cur->id != TNAME) return error("\nError at %d: Expected variable name", cur->line_no);
  consumeToken(cur);

  if (cur->id == TLSQPAREN) {
    consumeToken(cur);
    if (parseExpression() == ERROR) return ERROR;
    if (cur->id != TRSQPAREN) return error("\nError at %d: Expected ']'", cur->line_no);
    consumeToken(cur);
  }
  return NORMAL;
}

static int parseInput()
{
  if (cur->id != TREAD && cur->id != TREADLN)
    return error("\nError at %d: Expected 'read' or 'readln'", cur->line_no);
  consumeToken(cur);
  if (cur->id == TLPAREN) {
    consumeToken(cur);
    has_space = true;
    if (parseVar() == ERROR) return ERROR;
    while (cur->id == TCOMMA) {
      consumeToken(cur);
      if (parseVar() == ERROR) return ERROR;
    }
    if (cur->id != TRPAREN) return error("\nError at %d: Expected ')'", cur->line_no);
    consumeToken(cur);
  }
  return NORMAL;
}

static int parseOutputFormat()
{
  if (cur->id == TSTRING && strlen(cur->str) != 1) {
    consumeToken(cur);
  } else if (parseExpression() == ERROR) {
    return ERROR;
  }
  return NORMAL;
}

static int parseOutputStatement()
{
  if (cur->id != TWRITE && cur->id != TWRITELN)
    return error("\nError at %d: Expected 'write' or 'writeln'", cur->line_no);
  consumeToken(cur);

  if (cur->id != TLPAREN) return NORMAL;
  consumeToken(cur);
  has_space = true;
  if (parseOutputFormat() == ERROR) return ERROR;
  while (cur->id == TCOMMA) {
    consumeToken(cur);
    if (parseOutputFormat() == ERROR) return ERROR;
  }
  if (cur->id != TRPAREN) return error("\nError at %d: Expected ')'", cur->line_no);
  consumeToken(cur);
  return NORMAL;
}

static int parseStatement()
{
  switch (cur->id) {
    // 代入文
    case TNAME:
      if (parseAssignment() == ERROR) return ERROR;
      break;
    // 分岐文
    case TIF:
      if (parseCondition() == ERROR) return ERROR;
      break;
    // 繰り返し文
    case TWHILE:
      if (parseIteration() == ERROR) return ERROR;
      break;
    // 脱出文
    case TBREAK:
      if (iteration_level == 0)
        return error("\nError at %d: 'break' statement not within loop", cur->line_no);
      consumeToken(cur);
      break;
    // 手続き呼び出し文
    case TCALL:
      if (parseCall() == ERROR) return ERROR;
      break;
    // 戻り文
    case TRETURN:
      consumeToken(cur);
      break;
    // 入力文
    case TREAD:
    case TREADLN:
      if (parseInput() == ERROR) return ERROR;
      break;
    // 出力文
    case TWRITE:
    case TWRITELN:
      if (parseOutputStatement() == ERROR) return ERROR;
      break;
    // 複合文
    case TBEGIN:
      indent_level++;
      if (parseCompoundStatement() == ERROR) return ERROR;
      break;

    default:
      // return error("\nError at %d: Expected statement", cur->line_no);
      break;
  }
  return NORMAL;
}

static int parseCompoundStatement()
{
  if (cur->id != TBEGIN) return error("\nError at %d: Expected 'begin'", cur->line_no);

  consumeToken(cur);
  indent_level++;
  if (parseStatement() == ERROR) return ERROR;
  while (cur->id == TSEMI) {
    consumeToken(cur);
    if (parseStatement() == ERROR) return ERROR;
  }
  printf("cur->id: %d", cur->id);
  if (cur->id != TEND) return error("\nError at %d: Expected 'end'", cur->line_no);
  indent_level--;
  consumeToken(cur);
  indent_level--;
  return NORMAL;
}

static int parseFormalParamters()
{
  if (cur->id != TLPAREN) return error("\nError at %d: Expected '('", cur->line_no);
  consumeToken(cur);
  has_space = true;
  if (parseVarNames()) return error("\nError at %d: Expected variable name", cur->line_no);
  if (cur->id != TCOLON) return error("\nError at %d: Expected ':'", cur->line_no);
  consumeToken(cur);
  if (parseType()) return error("\nError at %d: Expected type", cur->line_no);
  while (cur->id == TSEMI) {
    consumeToken(cur);
    if (parseVarNames()) return error("\nError at %d: Expected variable name", cur->line_no);
    if (cur->id != TCOLON) return error("\nError at %d: Expected ':'", cur->line_no);
    consumeToken(cur);
    if (parseType()) return error("\nError at %d: Expected type", cur->line_no);
  }
  if (cur->id != TRPAREN) return error("\nError at %d: Expected ')'", cur->line_no);

  return NORMAL;
}

static int parseSubProgram()
{
  if (cur->id != TPROCEDURE) return error("\nError at %d: Expected 'procedure'", cur->line_no);
  indent_level++;
  consumeToken(cur);
  if (cur->id != TNAME) return error("\nError at %d: Expected procedure name", cur->line_no);
  consumeToken(cur);

  if (cur->id == TLPAREN) parseFormalParamters();

  if (cur->id != TSEMI) return error("\nError at %d: Expected ';'", cur->line_no);
  consumeToken(cur);

  if (cur->id == TVAR)
    if (parseVar() == ERROR) return ERROR;

  if (parseCompoundStatement() == ERROR) return ERROR;

  if (cur->id != TSEMI) return error("\nError at %d: Expected ';'", cur->line_no);
  consumeToken(cur);
  return NORMAL;
}

static int parseBlock()
{
  for (;;) {
    if (cur->id == TVAR) {
      if (parseVarDeclaration() == ERROR) return ERROR;
    } else if (cur->id == TPROCEDURE) {
      if (parseSubProgram() == ERROR) return ERROR;
    } else
      break;
  }
  if (parseCompoundStatement() == ERROR) return ERROR;

  return NORMAL;
}

static int parseProgram()
{
  if (cur->id != TPROGRAM)
    return error("\nError at %d: Expected 'program' at the beginning of the program", cur->line_no);
  consumeToken(cur);

  if (cur->id != TNAME) return error("\nError at %d: Expected program name", cur->line_no);
  consumeToken(cur);

  if (cur->id != TSEMI)
    return error("\nError at %d: Expected ';' at the end of the program name.", cur->line_no);
  consumeToken(cur);

  if (parseBlock() == ERROR) return ERROR;
  if (cur->id != TDOT) return error("\nError at %d: Expected '.'", cur->line_no);
  consumeToken(cur);

  return NORMAL;
}

void parse(Token * tok)
{
  cur = tok;
  parseProgram();
}
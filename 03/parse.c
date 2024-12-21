#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "lpp.h"
#define HASHSIZE 1000
/**
 * @brief 段付の深さを表す変数
 * 初期値は0であり、段付が深くなるごとにインクリメントされる
 */
static int indent_level = 0;

//! 行頭にいるかどうかを表す変数。トークンのリストにも改行されて段落の一番最初にあるかどうかを表すフラグがあるが、この変数は文脈に応じて改行されるべきかを判断するための変数である。トークンに付与されたものとこれの二つのフラグの論理和で改行されるべきかを判断する。
static bool at_bol = false;

//! 現在注目しているトークン
static Token * cur;

//! 現在注目しているトークンの位置を表す変数
static int iteration_level = 0;

//! クロスリファレンス表を格納するHashMap
HashMap *globalid, *localid;

//! 現在指しているクロスリファレンス表のポインタを格納している変数
HashMap ** current_id;

//! 定義されたプロシージャの名前を格納する変数
static char * procname = NULL;

//! 副プログラムの仮引数のカウント用変数
static uint parameter_num = 0;

typedef struct
{
  int line_no;
  char * varname;
} VAR;

DECLARE_STACK(VARNAME, VAR)
VARNAME varname_stack;

TYPE * type = NULL;
ID * node = NULL;

//! 定義された行番号を格納する変数

//! トークンの種類を表す文字列の配列
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

static TYPE * copyType(TYPE * src)
{
  if (!src) return NULL;
  TYPE * dst = malloc(sizeof(TYPE));
  *dst = *src;
  dst->is_freed = false;
  dst->etp = copyType(src->etp);
  dst->paratp = copyType(src->paratp);
  return dst;
}

static TYPE_KIND decodeIDtoTYPEKIND(int id, bool is_array)
{
  if (id == TPROCEDURE) return TPPROC;
  if (!is_array) {
    switch (id) {
      case TINTEGER:
      case TNUMBER:
        return TPRINT;
      case TCHAR:
      case TSTRING:
        return TPCHAR;
      case TTRUE:
      case TFALSE:
      case TBOOLEAN:
        return TPBOOL;
      default:
        return error("\nhogeError at %d: Expected type", cur->line_no);
    }
  } else {
    switch (id) {
      case TINTEGER:
      case TNUMBER:
        return TPARRAYINT;
      case TCHAR:
      case TSTRING:
        return TPARRAYCHAR;
      case TTRUE:
      case TFALSE:
      case TBOOLEAN:
        return TPARRAYBOOL;
      default:
        return error("\nhogeError at %d: Expected type", cur->line_no);
    }
  }
}

static TokenID decodeTYPEKINDtoID(int typekind)
{
  switch (typekind) {
    case TPRINT:
    case TPARRAYINT:
      return TINTEGER;
    case TPCHAR:
    case TPARRAYCHAR:
      return TCHAR;
    case TPBOOL:
    case TPARRAYBOOL:
      return TBOOLEAN;
    default:
      return error("\nError at %d: Invalid typekind %d", cur->line_no, typekind);
  }
}

static void pushIref(LINE ** iref, int refline)
{
  while (*iref != NULL) {
    iref = &(*iref)->nextlinep;
  }
  *iref = malloc(sizeof(LINE));
  (*iref)->reflinenum = refline;
  (*iref)->nextlinep = NULL;
}

static ID * lookupAndAddIref(const char * name, int line_no)
{
  ID * entry = NULL;
  entry = getValueFromHashMap(*current_id, name);
  if (entry == NULL && *current_id == localid) {
    entry = getValueFromHashMap(globalid, name);
  }
  if (entry != NULL) {
    pushIref(&entry->irefp, line_no);
  }
  return entry;
}

static void printName(Entry * entry)
{
  printf("%s", entry->key);
  if (entry->value->procname != NULL) printf(":%s", entry->value->procname);
  printf("|");
}

static void printType(TYPE * tp)
{
  if (tp == NULL) return;
  switch (tp->ttype) {
    case TPPROC:
      printf("procedure");
      if (tp->paratp != NULL) {
        printf("(");
        TYPE * param = tp->paratp;
        while (param != NULL) {
          printf("%s", token_str[decodeTYPEKINDtoID(param->ttype)]);
          param = param->paratp;
          if (param != NULL) {
            printf(", ");
          }
        }
        printf(")");
      }
      break;
    case TPRINT:
    case TPCHAR:
    case TPBOOL:
      printf("%s", token_str[decodeTYPEKINDtoID(tp->ttype)]);
      break;
    default:
      printf("array[%d]of%s", tp->etp->arraysize, token_str[decodeTYPEKINDtoID(tp->etp->ttype)]);
      break;
  }
}

static void printCrossreferenceTable(HashMap * idroot)
{
  for (int i = 0; i < idroot->size; i++) {
    Entry * entry = idroot->entries[i];
    while (entry != NULL) {
      // クロスリファレンス表の出力
      printName(entry);
      printType(entry->value->itp);
      printf("|%d|", entry->value->defline);
      while (entry->value->irefp != NULL) {
        printf("%d", entry->value->irefp->reflinenum);
        entry->value->irefp = entry->value->irefp->nextlinep;
        if (entry->value->irefp != NULL) {
          printf(",");
        }
      }
      printf("\n");
      entry = entry->next;
    }
  }
}

/**
 * @brief ローカルなスコープに入る
 * currnet_idをlocalidに向けて、localidのハッシュマップの初期化する。
 */
static void enterScope()
{
  localid = newHashMap(HASHSIZE);
  current_id = &localid;
}

/**
 * @brief グローバルなスコープに入る
 * ハッシュマップの中身をクロスリファレンス表として出力する。
 * ローカルなハッシュマップはメモリから開放を行う。
 * 最後にcurrent_idをglobalidに向ける。
 */
static void exitScope()
{
  printCrossreferenceTable(localid);
  procname = NULL;
  freeHashMap(*current_id);
  current_id = &globalid;
}

/**
 * @brief indent_levelに応じて段付を行う
 * もしindent_levelが負の値になった時は異常と見なしてプログラムを終了させる
 * 
 */
static void printIndent()
{
  if (indent_level < 0) {
    error("Line %d: Indent level is negative", cur->line_no);
    exit(1);
  }
  for (int i = 0; i < indent_level; i++) {
    printf("    ");
  }
}

/**
 * @brief トークンの種類に応じてプリティプリントのやり方を変える
 * もしトークンのhas_spaceフラグがtrueかつトークンの位置が行頭以外であったとき
 * 半角の空白をトークンの前に挟む
 * @param tok 
 */
static void printToken(const Token * tok)
{
  // プリティプリント防止
  return;
  if (!at_bol && !tok->at_bol && tok->has_space) printf(" ");
  if ((tok->at_bol || at_bol) && tok->id != TPROGRAM) {
    printf("\n");
    printIndent();
  }

  if (tok->kind == TK_KEYWORD || tok->kind == TK_PUNCT) {
    printf("%s", token_str[cur->id]);
  } else if (cur->kind == TK_NUM) {
    printf("%s", cur->str);
  } else if (cur->kind == TK_STR) {
    printf("'%s'", cur->str);
  } else {
    printf("%s", cur->str);
  }
  fflush(stdout);
  at_bol = false;
}

static TYPE * newType(TYPE_KIND ttype, int arraysize, TYPE * etp, TYPE * paratp)
{
  TYPE * tp = malloc(sizeof(TYPE));
  tp->ttype = ttype;
  tp->arraysize = arraysize;
  tp->etp = etp;
  tp->paratp = paratp;
  tp->is_freed = false;
  return tp;
}

static void freeType(TYPE * tp)
{
  if (tp == NULL || tp->is_freed) return;  // NULL チェックと解放済みチェック

  tp->is_freed = true;  // 解放済みに設定
  if (tp->etp != NULL) freeType(tp->etp);
  if (tp->paratp != NULL) freeType(tp->paratp);
  free(tp);
}

static ID * newID(const char * name, const char * _procname, TYPE * itp, int ispara, int defline)
{
  ID * id = malloc(sizeof(ID));
  id->name = strdup(name);
  if (_procname != NULL)
    id->procname = strdup(_procname);
  else
    id->procname = NULL;
  id->itp = copyType(itp);
  id->ispara = ispara;
  id->irefp = NULL;
  id->defline = defline;
  return id;
}

static void freeID(ID * id)
{
  free(id->name);
  free(id->procname);
  free(id);
}

/**
 * @brief トークンを標準出力して現在注目しているトークンの位置を一つ進める
 * 
 * @param tok 
 */
static void consumeToken(Token * tok)
{
  printToken(tok);
  cur = cur->next;
}

/**
 * @brief もし現在注目しているトークンが関係演算子であった場合trueを返す
 * 
 * @return true 
 * @return false 
 */
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

/**
 * @brief もし現在注目しているトークンが乗法演算子であった場合trueを返す
 * 
 * @return true 
 * @return false 
 */
static bool isMulOp()
{
  switch (cur->id) {
    case TSTAR:
    case TDIV:
    case TAND:
      return true;
    default:
      return false;
  }
}

/**
 * @brief もし現在注目しているトークンが加法演算子であった場合trueを返す
 * 
 * @return true 
 * @return false 
 */
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

/**
 * @brief もし現在注目しているトークンが標準型であった場合その型表す定数を返す
 * 
 * @return TINTEGER
  * @return TBOOLEAN
  * @return TCHAR
  * @return false
 */
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

/**
 * @brief 型であるかを確かめる
 * 
 * @return int 
 */
static int parseType()
{
  int vartype;
  if (isStdType()) {
    vartype = decodeIDtoTYPEKIND(cur->id, false);
    type = newType(vartype, -1, NULL, NULL);
    consumeToken(cur);
  } else if (cur->id == TARRAY) {
    int arraysize = 0;
    consumeToken(cur);
    if (cur->id != TLSQPAREN) return error("\nError at %d: Expected '['", cur->line_no);
    consumeToken(cur);
    if (cur->id != TNUMBER) return error("\nError at %d: Expected number", cur->line_no);
    arraysize = cur->num;
    consumeToken(cur);
    if (cur->id != TRSQPAREN) return error("\nError at %d: Expected ']'", cur->line_no);
    consumeToken(cur);
    if (cur->id != TOF) return error("\nError at %d: Expected 'of'", cur->line_no);
    consumeToken(cur);

    if (!isStdType()) return error("\nError at %d: Expected type", cur->line_no);
    vartype = decodeIDtoTYPEKIND(cur->id, true);

    TYPE * etp = newType(vartype, arraysize, NULL, false);
    type = newType(-1, -1, etp, NULL);
    consumeToken(cur);
  } else {
    return error("\nError at %d: Expected type", cur->line_no);
  }

  return NORMAL;
}

/**
 * @brief 変数名の並びであるかを確かめる
 * 
 * @return int 
 */
static int parseVarNames()
{
  if (cur->id != TNAME) return ERROR;
  parameter_num++;
  VAR var = {cur->line_no, cur->str};
  VARNAME_push(&varname_stack, var);
  consumeToken(cur);
  while (cur->id == TCOMMA) {
    consumeToken(cur);
    if (cur->id != TNAME) return error("\nError at %d: Expected variable name", cur->line_no);
    parameter_num++;
    VAR var = {cur->line_no, cur->str};
    VARNAME_push(&varname_stack, var);
    consumeToken(cur);
  }

  return NORMAL;
}

static void processVarNameStack()
{
  while (!VARNAME_is_empty(&varname_stack)) {
    VAR var = VARNAME_pop(&varname_stack);
    ID * value = getValueFromHashMap(*current_id, var.varname);
    if (value == NULL) {
      node = newID(var.varname, procname, type, false, var.line_no);
      insertToHashMap(*current_id, var.varname, node);
    } else {
      LINE * last_line_ptr = value->irefp;
      while (last_line_ptr != NULL) {
        last_line_ptr = last_line_ptr->nextlinep;
      }
      last_line_ptr->reflinenum = var.line_no;
    }
  }
}

/**
 * @brief 変数宣言部であるかを確かめる
 * 
 * @return int 
 */
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
  processVarNameStack();
  consumeToken(cur);

  while (cur->id == TNAME) {
    at_bol = true;
    if (parseVarNames() == ERROR)
      return error("\nError at %d: Expected variable name", cur->line_no);
    if (cur->id != TCOLON) return error("\nError at %d: Expected ':'", cur->line_no);
    consumeToken(cur);

    if (parseType() == ERROR) return ERROR;

    if (cur->id != TSEMI) return error("\nError at %d: Expected ';'", cur->line_no);
    processVarNameStack();
    consumeToken(cur);
  }
  indent_level -= 2;

  return NORMAL;
}

/**
 * @brief 項であるかを確かめる
 * 
 * @return int 
 */
static int parseTerm()
{
  if (parseFactor() == ERROR) return ERROR;

  while (isMulOp()) {
    consumeToken(cur);
    if (parseFactor() == ERROR) return ERROR;
  }
  return NORMAL;
}

/**
 * @brief 単純式であるかを確かめる
 * 
 * @return int 
 */
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

/**
 * @brief 式であるかを確かめる
 * 
 * @return int 
 */
static int parseExpression()
{
  if (parseSimpleExpression() == ERROR) return ERROR;
  while (isparseRelOp()) {
    consumeToken(cur);

    if (parseSimpleExpression() == ERROR) return ERROR;
  }
  return NORMAL;
}

/**
 * @brief 因子であるかを確かめる
 * 
 * @return int 
 */
static int parseFactor()
{
  switch (cur->id) {
    // 変数
    case TNAME: {
      // ID * entry = lookupAndAddIref(cur->str, cur->line_no);
      // if (entry == NULL) {
      //   return error("\nError at %d: Undefined variable name '%s'", cur->line_no, cur->str);
      // }
      if (parseVar() == ERROR) return ERROR;
    } break;
    // 定数
    case TNUMBER:
      type->ttype = TPRINT;
      consumeToken(cur);
      break;
    case TFALSE:
    case TTRUE:
      type->ttype = TPBOOL;
      consumeToken(cur);
      break;
    case TSTRING:
      type->ttype = TPCHAR;
      consumeToken(cur);
      break;
    case TLPAREN:
      consumeToken(cur);
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
      type->ttype = cur->id;
      consumeToken(cur);
      if (cur->id != TLPAREN) error("\nError at %d: Expected '('", cur->line_no);
      consumeToken(cur);
      if (parseExpression() == ERROR) return ERROR;
      if (cur->id != TRPAREN) error("\nError at %d: Expected ')'", cur->line_no);
      consumeToken(cur);
      break;
    default:
      return error("\nError at %d: Expected factor", cur->line_no);
      break;
  }
  return NORMAL;
}

/**
 * @brief 代入文であるかを確かめる
 * 
 * @return int 
 */
static int parseAssignment()
{
  if (parseVar() == ERROR) return ERROR;

  if (cur->id != TASSIGN) return error("\nError at %d: Expected ':='", cur->line_no);
  consumeToken(cur);

  if (parseExpression() == ERROR) return ERROR;
  return NORMAL;
}

/**
 * @brief 条件文であるかを確かめる
 * 
 * @return int 
 */
static int parseCondition()
{
  if (cur->id != TIF) return error("\nError at %d: Expected 'if'", cur->line_no);
  consumeToken(cur);
  if (parseExpression() == ERROR) return ERROR;
  if (cur->id != TTHEN) return error("\nError at %d: Expected 'then'", cur->line_no);
  consumeToken(cur);
  at_bol = true;
  bool is_begin = cur->id == TBEGIN;
  if (!is_begin) indent_level++;
  if (parseStatement() == ERROR) return ERROR;
  if (!is_begin) indent_level--;
  if (cur->id == TELSE) {
    consumeToken(cur);
    at_bol = true;
    is_begin = cur->id == TBEGIN;
    if (!is_begin) indent_level++;

    if (parseStatement() == ERROR) return ERROR;
    if (!is_begin) indent_level--;
  }
  return NORMAL;
}

/**
 * @brief 繰り返し文であるかを確かめる
 * 
 * @return int 
 */
static int parseIteration()
{
  if (cur->id != TWHILE) return error("\nError at %d: Expected 'while'", cur->line_no);
  consumeToken(cur);
  if (parseExpression() == ERROR) return ERROR;
  if (cur->id != TDO) return error("\nError at %d: Expected 'do'", cur->line_no);
  consumeToken(cur);
  iteration_level++;
  if (parseStatement() == ERROR) return ERROR;
  return NORMAL;
}

/**
 * @brief 手続き呼び出し文であるかを確かめる
 * 
 * @return int 
 */
static int parseCall()
{
  if (cur->id != TCALL) return error("\nError at %d: Expected 'call'", cur->line_no);
  consumeToken(cur);
  if (cur->id != TNAME) return error("\nError at %d: Expected procedure name", cur->line_no);
  ID * entry = lookupAndAddIref(cur->str, cur->line_no);

  if (procname != NULL && strcmp(procname, cur->str) == 0) {
    return error("\nError at %d: Recursive call", cur->line_no);
  }

  if (entry == NULL || entry->itp->ttype != TPPROC)
    return error("\nError at %d: Undefined procedure name %s", cur->line_no);

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

/**
 * @brief 変数であるかを確かめる
 * 
 * @return int 
 */
static int parseVar()
{
  if (cur->id != TNAME) return error("\nError at %d: Expected variable name", cur->line_no);
  ID * entry = lookupAndAddIref(cur->str, cur->line_no);

  consumeToken(cur);

  if (cur->id == TLSQPAREN) {
    consumeToken(cur);
    if (parseExpression() == ERROR) return ERROR;
    if (cur->id != TRSQPAREN) return error("\nError at %d: Expected ']'", cur->line_no);
    consumeToken(cur);
  }
  return NORMAL;
}

/**
 * @brief 入力文であるかを確かめる
 * 
 * @return int 
 */
static int parseInput()
{
  if (cur->id != TREAD && cur->id != TREADLN)
    return error("\nError at %d: Expected 'read' or 'readln'", cur->line_no);
  consumeToken(cur);
  if (cur->id == TLPAREN) {
    consumeToken(cur);
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

/**
 * @brief 出力指定子であるかを確かめる
 * 
 * @return int 
 */
static int parseOutputFormat()
{
  if (cur->id == TSTRING && cur->len != 1) {
    consumeToken(cur);
  } else if (parseExpression() != ERROR) {
    if (cur->id != TCOLON) return NORMAL;
    consumeToken(cur);
    if (cur->id != TNUMBER) return error("\nError at %d: Expected number", cur->line_no);
    consumeToken(cur);
  } else
    return ERROR;
  return NORMAL;
}

/**
 * @brief 出力文であるかを確かめる
 * 
 * @return int 
 */
static int parseOutputStatement()
{
  if (cur->id != TWRITE && cur->id != TWRITELN)
    return error("\nError at %d: Expected 'write' or 'writeln'", cur->line_no);
  consumeToken(cur);

  if (cur->id != TLPAREN) return NORMAL;
  consumeToken(cur);
  if (parseOutputFormat() == ERROR) return ERROR;

  while (cur->id == TCOMMA) {
    consumeToken(cur);
    if (parseOutputFormat() == ERROR) return ERROR;
  }
  if (cur->id != TRPAREN) return error("\nError at %d: Expected ')'", cur->line_no);
  consumeToken(cur);
  return NORMAL;
}

/**
 * @brief 文であるかを確かめる
 * 
 * @return int 
 */
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
      indent_level--;
      break;

    default:
      // 空文
      break;
  }
  return NORMAL;
}

/**
 * @brief 複合文であるかを確かめる
 * 
 * @return int 
 */
static int parseCompoundStatement()
{
  if (cur->id != TBEGIN) return error("\nError at %d: Expected 'begin'", cur->line_no);

  consumeToken(cur);
  indent_level++;
  at_bol = true;

  if (parseStatement() == ERROR) return ERROR;
  while (cur->id == TSEMI) {
    consumeToken(cur);
    at_bol = true;
    if (parseStatement() == ERROR) return ERROR;
  }
  if (cur->id != TEND) return error("\nError at %d: Expected 'end'", cur->line_no);
  indent_level--;
  consumeToken(cur);

  return NORMAL;
}

/**
 * @brief 手続きの仮引数の型リストを登録する関数
 * 
 * @param procname 手続き名
 * @param param_type_list 仮引数の型リストの先頭
 * @return int 正常終了の場合は NORMAL、エラーの場合は ERROR を返す
 */
static int registerProcedureParameters(const char * procname, TYPE * param_type_list)
{
  // 手続きの ID を取得
  ID * procnode = getValueFromHashMap(globalid, procname);
  if (procnode == NULL) return error("\nError at %d: Undefined procedure name", cur->line_no);

  // 仮引数の型リストを設定
  procnode->itp->paratp = param_type_list;
  return NORMAL;
}

/**
 * @brief 仮引数の並びを解析する関数
 * 
 * @return int 正常終了の場合は NORMAL、エラーの場合は ERROR を返す
 */
static int parseFormalParamters()
{
  if (cur->id != TLPAREN) return error("\nError at %d: Expected '('", cur->line_no);
  consumeToken(cur);

  TYPE * param_type_list = NULL;  // 仮引数の型リストの先頭
  TYPE * last_param_type = NULL;  // 仮引数の型リストの末尾

  do {
    if (parseVarNames() == ERROR)
      return error("\nError at %d: Expected variable name", cur->line_no);

    if (cur->id != TCOLON) return error("\nError at %d: Expected ':'", cur->line_no);
    consumeToken(cur);

    if (!isStdType()) return error("\nError at %d: Expected type", cur->line_no);
    int vartype = decodeIDtoTYPEKIND(cur->id, false);
    consumeToken(cur);

    // 仮引数名ごとに型を設定
    while (!VARNAME_is_empty(&varname_stack)) {
      VAR var = VARNAME_pop(&varname_stack);
      TYPE * param_type = newType(vartype, -1, NULL, NULL);

      // 型リストに追加
      if (param_type_list == NULL) {
        param_type_list = param_type;
      } else {
        last_param_type->paratp = param_type;
      }
      last_param_type = param_type;

      // 仮引数をハッシュマップに登録
      if (getValueFromHashMap(localid, var.varname) == NULL) {
        ID * param_id = newID(var.varname, procname, param_type, true, var.line_no);
        insertToHashMap(localid, var.varname, param_id);
      }
    }

  } while (cur->id == TSEMI && (consumeToken(cur), true));

  if (cur->id != TRPAREN) return error("\nError at %d: Expected ')'", cur->line_no);
  consumeToken(cur);

  // 仮引数の型リストを登録する関数を呼び出す
  if (registerProcedureParameters(procname, param_type_list) == ERROR) return ERROR;

  return NORMAL;
}

/**
 * @brief 副プログラム宣言であるかを確かめる
 * 
 * @return int 
 */
static int parseSubProgram()
{
  if (cur->id != TPROCEDURE) return error("\nError at %d: Expected 'procedure'", cur->line_no);
  // ローカルなスコープに入る
  enterScope();
  indent_level = 1;
  consumeToken(cur);

  if (cur->id != TNAME) return error("\nError at %d: Expected procedure name", cur->line_no);

  type = newType(TPPROC, -1, NULL, NULL);
  node = newID(cur->str, NULL, type, false, cur->line_no);
  insertToHashMap(globalid, cur->str, node);

  procname = strdup(cur->str);

  consumeToken(cur);

  if (cur->id == TLPAREN) parseFormalParamters();

  if (cur->id != TSEMI) return error("\nError at %d: Expected ';'", cur->line_no);
  consumeToken(cur);

  if (cur->id == TVAR)
    if (parseVarDeclaration() == ERROR) return ERROR;

  if (parseCompoundStatement() == ERROR) return ERROR;

  if (cur->id != TSEMI) return error("\nError at %d: Expected ';'", cur->line_no);
  consumeToken(cur);
  indent_level--;
  exitScope();
  return NORMAL;
}

/**
 * @brief ブロックであるかを確かめる
 * 
 * @return int 
 */
static int parseBlock()
{
  for (;;) {
    if (cur->id == TVAR) {
      // 変数宣言部は行頭から1段落付けされる。
      // parseVarDeclaration内でインクリメントされるので0にして辻褄を合わせる。
      indent_level = 0;
      if (parseVarDeclaration() == ERROR) return ERROR;
    } else if (cur->id == TPROCEDURE) {
      if (parseSubProgram() == ERROR) return ERROR;
    } else
      break;
  }
  if (parseCompoundStatement() == ERROR) return ERROR;

  return NORMAL;
}

/**
 * @brief プログラムであるかを確かめる
 * 
 * @return int 
 */
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

/**
 * @brief 構文解析を行う関数
 * 
 * @param tok トークンの連結リストの先頭を示すポインタ
 */
void parse(Token * tok)
{
  cur = tok;
  globalid = newHashMap(HASHSIZE);
  current_id = &globalid;
  VARNAME_init(&varname_stack);
  if (parseProgram() == ERROR) error("Parser aborted with error.");
  printCrossreferenceTable(globalid);
  freeHashMap(globalid);
}
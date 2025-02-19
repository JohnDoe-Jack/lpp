#include <stdlib.h>
#include <string.h>

#include "lpp.h"
static FILE * output_file;
static Token * cur;

//! 定義されたプロシージャの名前を格納する変数
static char * procname = NULL;

//! アドレスをロードする必要があるかを表す変数
static bool needs_address_load = false;

//! 行頭かどうかを表す変数
static bool at_bol = false;

//! パラメータかどうかを表す変数
static bool is_parameter = false;

//! アドレスをロードしたかを表す変数
static bool loaded_address = false;

//! 呼び出し変数の名前を格納する変数
static char * call_var_name = NULL;

//! プリントする文字列を格納する変数
static char * print_buf = NULL;

//! トークンの種類を表す文字列の配列
static const char * token_str[NUMOFTOKEN + 1] = {
  "",       "NAME",   "program",   "var",     "array",   "of",     "begin",   "end",  "if",
  "then",   "else",   "procedure", "return",  "call",    "while",  "do",      "not",  "or",
  "div",    "and",    "char",      "integer", "boolean", "readln", "writeln", "true", "false",
  "NUMBER", "STRING", "+",         "-",       "*",       "=",      "<>",      "<",    "<=",
  ">",      ">=",     "(",         ")",       "[",       "]",      ":=",      ".",    ",",
  ":",      ";",      "read",      "write",   "break"};

DECLARE_STACK(PARAMETER, char *);

//! 副プログラムの仮引数のカウント用変数
PARAMETER parameter_stack;

//! シンボルテーブルの構造体
typedef struct Symbol Symbol;

//! シンボルテーブルの構造体
struct Symbol
{
  //! キー
  char * key;
  //! ラベル
  char * label;
  //! 型
  char * type;
  //! パラメータかどうか
  bool ispara;
};

Symbol * symbols;

//! 式の評価結果を表現する構造体
typedef struct Obj * Obj;

//! 式の評価結果を表現する構造体
struct Obj
{
  //! 型
  TYPE_KIND type;
  //! 左辺値を持つ式かどうか
  bool isLVal;
};

static int pCompoundStatement();
static int pStatement();
static Obj pExpression();

//! Symbol構造体から一意に定まる変数名とプロシージャの組み合わせからsymbols配列から適切なsymbolを取得する関数
static Symbol getSymbol(char * key)
{
  for (int i = 0; symbols[i].key != NULL; i++) {
    if (strcmp(symbols[i].key, key) == 0) {
      return symbols[i];
    }
  }
  Symbol symbol = {NULL, NULL, NULL, false};
  return symbol;
}

//! output_fileに文字列を改行付きで出力する関数
static void println(char * fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  vfprintf(output_file, fmt, ap);
  va_end(ap);
  fprintf(output_file, "\n");
}

//! 文字列で表現された記号表を'|'で区切って破壊的な代入を行った後にSymbol構造体の配列に変換する関数
static Symbol * parseSymbols(const SymbolBuffer * buf)
{
  Symbol * symbols = malloc(sizeof(Symbol) * buf->line_count);
  char * p = buf->buf;
  for (int i = 0; i < buf->line_count; i++) {
    symbols[i].key = p;
    while (*p != '|') p++;
    *p++ = '\0';
    symbols[i].label = p;
    while (*p != '|') p++;
    *p++ = '\0';
    symbols[i].type = p;
    while (*p != '|') p++;
    *p++ = '\0';
    symbols[i].ispara = *p == '1';
    while (*p != '\n') p++;
    p++;
  }
  return symbols;
}

//! print_bufに文字列を格納していき、改行するタイミングでoutput_fileに出力する関数
static void printToken(const Token * tok)
{
  if (print_buf == NULL) {
    print_buf = malloc(sizeof(char) * 256);
    print_buf[0] = '\0';
    strcat(print_buf, ";\t");
  }
  if (!at_bol && !tok->at_bol && tok->has_space) strcat(print_buf, " ");

  if ((at_bol || tok->at_bol) && tok->id != TPROGRAM) {
    println("%s", print_buf);
    print_buf[0] = '\0';
    strcat(print_buf, ";\t");
  }

  if (tok->kind == TK_KEYWORD || tok->kind == TK_PUNCT) {
    strcat(print_buf, token_str[tok->id]);
  } else if (tok->kind == TK_NUM) {
    strcat(print_buf, tok->str);
  } else if (tok->kind == TK_STR) {
    strcat(print_buf, "'");
    strcat(print_buf, tok->str);
    strcat(print_buf, "'");
  } else {
    strcat(print_buf, tok->str);
  }
  at_bol = false;
}

//! トークンを一つ進める関数
static void consumeToken()
{
  printToken(cur);
  cur = cur->next;
}

//! ラベルの番号を生成して返す関数
static int getLabelNum()
{
  static int labelcounter = 1;
  return labelcounter++;
}

//! ラベルを生成する関数
static void genLabel(int label) { println("L%04d", label); }

//! 命令を生成する関数
static void genCode(char * opc, char * opr) { println("\t%s\t%s", opc, opr); }

//! ラベル付きの命令を生成する関数
static void genCodeLabel(char * opc, int label) { println("\t%s\tL%04d", opc, label); }

//! 配列化どうかを判定する関数
static bool isArray(Symbol key) { return key.type[0] == 'a'; }

//! 配列のサイズを型を表す文字列から取得する関数
static int getArraySize(char * type)
{
  const char * p = strchr(type, '[');
  p++;
  char buf[64];
  int i = 0;
  while (*p != ']') {
    buf[i++] = *p;
    p++;
  }
  buf[i] = '\0';
  return atoi(buf);
}

//! 変数の並びから命令を生成する関数
static int pVarNames(bool isparam)
{
  Symbol symbol;
  char key[256];
  if (procname != NULL) {
    snprintf(key, sizeof(key), "%s:%s", cur->str, procname);
  } else {
    strcpy(key, cur->str);
  }
  symbol = getSymbol(key);
  if (isArray(symbol)) {
    if (isparam) PARAMETER_push(&parameter_stack, symbol.label);
    println("%s\tDS\t%d", symbol.label, getArraySize(symbol.type));
  } else {
    if (symbol.label == NULL) {
      return error("Error at %d: Undefined variable %s", cur->line_no, cur->str);
    }
    if (isparam) PARAMETER_push(&parameter_stack, symbol.label);
    println("%s\tDC\t0", symbol.label);
  }
  consumeToken();
  while (cur->id == TCOMMA) {
    consumeToken();
    if (procname != NULL) {
      snprintf(key, sizeof(key), "%s:%s", cur->str, procname);
    } else {
      strcpy(key, cur->str);
    }
    symbol = getSymbol(key);
    if (isArray(symbol)) {
      if (isparam) PARAMETER_push(&parameter_stack, symbol.label);
      println("%s\tDS\t%d", symbol.label, getArraySize(symbol.type));
    } else {
      if (symbol.label == NULL)
        return error("Error at %d: Undefined variable %s", cur->line_no, cur->str);
      if (isparam) PARAMETER_push(&parameter_stack, symbol.label);
      println("%s\tDC\t0", symbol.label);
      consumeToken();
    }
  }
  return NORMAL;
}

//! 変数宣言部から命令を生成する関数
static int pVarDeclaration()
{
  consumeToken();
  at_bol = true;
  pVarNames(false);
  while (cur->id != TSEMI) consumeToken();
  consumeToken();

  while (cur->id == TNAME) {
    at_bol = true;
    pVarNames(false);
    while (cur->id != TSEMI) consumeToken();
    consumeToken();
  }

  return NORMAL;
}

//! 型を表す文字列から型を表すenum定数を返す関数
static int decodeType(char * type)
{
  if (strncmp(type, "array", 5) == 0) {
    char * p = strchr(type, '[');
    if (!p) return -1;

    while (*p != ']') {
      if (*p == '\0') return -1;
      p++;
    }
    p++;

    if (strncmp(p, "of", 2) != 0) return -1;
    p += 2;

    while (isspace(*p)) p++;

    if (strncmp(p, "integer", 7) == 0) return TPINT;
    if (strncmp(p, "boolean", 7) == 0) return TPBOOL;
    if (strncmp(p, "char", 4) == 0) return TPCHAR;
    return -1;
  }
  if (strcmp(type, "integer") == 0) return TPINT;
  if (strcmp(type, "boolean") == 0) return TPBOOL;
  if (strcmp(type, "char") == 0) return TPCHAR;
  return -1;
}

//! 変数から命令を生成する関数
static int pVar()
{
  Symbol symbol;
  if (procname != NULL) {
    char key[256];
    snprintf(key, sizeof(key), "%s:%s", cur->str, procname);
    symbol = getSymbol(key);
    if (symbol.label == NULL) symbol = getSymbol(cur->str);
    if (symbol.label == NULL)
      return error("Error at %d: Undefined variable %s", cur->line_no, cur->str);
  } else {
    symbol = getSymbol(cur->str);
  }
  consumeToken();
  if (cur->id == TLSQPAREN) {
    consumeToken();
    // Expressionの結果はGR1に格納されている
    bool isAddress2 = needs_address_load;
    needs_address_load = false;
    if (pExpression() == NULL) return ERROR;
    needs_address_load = isAddress2;
    // 配列の添字が0より大きいかをチェック(GR0には0が常に格納されている)
    genCode("CPA", "GR1,GR0");
    genCode("JMI", "EROV");
    // 配列の添字が配列のサイズより小さいかをチェック
    println("\tLAD\tGR2,%d", getArraySize(symbol.type) - 1);
    genCode("CPA", "GR1,GR2");
    genCode("JPL", "EROV");
    // GR1の分offsetを考慮して配列にアクセスする

    if (needs_address_load && !symbol.ispara) {
      println("\tLAD\tGR1,%s,GR1", symbol.label);
    } else {
      println("\tLD\tGR1,%s,GR1", symbol.label);
    }

    if (cur->id != TRSQPAREN) {
      return error("Error at %d: Expected ']'", cur->line_no);
    }
    consumeToken();
  } else {
    if (symbol.label == NULL) {
      return error("Error at %d: Undefined variable %s", cur->line_no, cur->str);
    }
    if (needs_address_load && !symbol.ispara) {
      println("\tLAD\tGR1,%s", symbol.label);
      loaded_address = true;
    } else {
      println("\tLD\tGR1,%s", symbol.label);
      loaded_address = false;
    }
  }
  int type = decodeType(symbol.type);
  if (type == -1) return error("Error at %d: Undefined type %s", cur->line_no, symbol.type);
  is_parameter = symbol.ispara;
  call_var_name = symbol.label;
  return type;
}

//! 代入文から命令を生成する関数
static int pAssignment()
{
  Obj lhs;
  needs_address_load = true;
  if (pVar() == ERROR) return ERROR;
  needs_address_load = false;
  genCode("PUSH", "0,GR1");

  if (cur->id != TASSIGN) return error("Error at %d: Expected ':='", cur->line_no);
  consumeToken();

  // Expressionの結果はGR1に格納されている
  if ((lhs = pExpression()) == NULL) return ERROR;

  // 左辺部の変数のアドレスをスタックからPOP
  genCode("POP", "GR2");
  // GR2には変数のアドレスが格納されているので、そのアドレスにGR1の値を格納する
  genCode("ST", "GR1,0,GR2");

  return NORMAL;
}

//! boolean型に型変換が行われた時にGR1に格納されている値からTrueかFalseかを判定して格納する関数
static void genStoreBoolean()
{
  int labelTrue = getLabelNum();
  int labelEnd = getLabelNum();
  genCode("CPA", "GR1,GR0");
  genCodeLabel("JZE", labelTrue);  // GR1に格納されている値が0ならばJUMPする
  genCode("LAD", "GR1,1");
  genCodeLabel("JUMP", labelEnd);
  genLabel(labelTrue);
  genCode("LAD", "GR1,0");
  genLabel(labelEnd);
}

//! 因子から命令を生成する関数
static Obj pFactor()
{
  Obj factor, expression;
  factor = malloc(sizeof(struct Obj));
  expression = malloc(sizeof(struct Obj));
  switch (cur->id) {
    // 変数
    case TNAME:
      factor->isLVal = true;
      if ((factor->type = pVar()) == TPRERROR) return NULL;
      if (is_parameter || loaded_address) genCode("LD", "GR1,0,GR1");
      break;
    // 定数
    case TNUMBER:
      factor->type = TPINT;
      factor->isLVal = false;
      println("\tLAD\tGR1,%d", cur->num);
      consumeToken();
      break;
    case TFALSE:
      factor->type = TPBOOL;
      factor->isLVal = false;
      println("\tLAD\tGR1,0");
      consumeToken();
      break;
    case TTRUE:
      factor->type = TPBOOL;
      factor->isLVal = false;
      println("\tLAD\tGR1,1");
      consumeToken();
      break;
    case TSTRING:
      factor->type = TPCHAR;
      factor->isLVal = false;
      println("\tLAD\tGR1,%d", (int)*cur->str);
      consumeToken();
      break;
      // "(" Expression ")"
    case TLPAREN:
      consumeToken();
      if ((factor = pExpression()) == NULL) return NULL;
      if (cur->id != TRPAREN) {
        error("Error at %d: Expected ')'", cur->line_no);
        return NULL;
      }
      consumeToken();
      break;
    case TNOT:
      consumeToken();
      if ((factor = pFactor()) == NULL) return NULL;
      genCode("LAD", "GR2,1");
      genCode("XOR", "GR1,GR2");
      break;
      // 標準型 "(" Expression ")"
    case TINTEGER:
      factor->type = TPINT;
      consumeToken();
      if (cur->id != TLPAREN) {
        error("Error at %d: Expected '('", cur->line_no);
        return NULL;
      }
      consumeToken();
      if ((expression = pExpression()) == NULL) return NULL;

      if (cur->id != TRPAREN) {
        error("Error at %d: Expected ')'", cur->line_no);
        return NULL;
      }
      consumeToken();
      factor->isLVal = expression->isLVal;
      break;
    case TBOOLEAN:
      factor->type = TPBOOL;
      consumeToken();
      if (cur->id != TLPAREN) {
        error("Error at %d: Expected '('", cur->line_no);
        return NULL;
      }

      consumeToken();
      if ((expression = pExpression()) == NULL) return NULL;

      switch (expression->type) {
        case TPINT: {
          int label = getLabelNum();
          genCode("CPA", "GR1,GR0");
          genCodeLabel("JZE", label);
          genCode("LAD", "GR1,1");
          genLabel(label);
          break;
        }
        case TPBOOL:
          break;
        case TPCHAR:
          genStoreBoolean();
          break;
        default:
          error("Error at %d: Expected boolean", cur->line_no);
          return NULL;
      }
      if (cur->id != TRPAREN) {
        error("Error at %d: Expected ')'", cur->line_no);
        return NULL;
      }
      consumeToken();
      factor->isLVal = expression->isLVal;
      break;
    case TCHAR:
      factor->type = TPCHAR;
      consumeToken();
      if (cur->id != TLPAREN) {
        error("Error at %d: Expected '('", cur->line_no);
        return NULL;
      }
      consumeToken();
      if ((expression = pExpression()) == NULL) return NULL;

      switch (expression->type) {
        case TPINT:
          genCode("LAD", "GR2,#007F");
          genCode("AND", "GR1,GR2");
          genCode("LAD", "GR2,0");
          break;
        case TPBOOL:
          genStoreBoolean();
          break;
        case TPCHAR:
          break;
        default:
          error("Error at %d: Expected char", cur->line_no);
          return NULL;
      }

      if (cur->id != TRPAREN) {
        error("Error at %d: Expected ')'", cur->line_no);
        return NULL;
      }
      consumeToken();
      factor->isLVal = expression->isLVal;
      break;

    default:
      error("Error at %d: Expected factor", cur->line_no);
      return NULL;
  }
  return factor;
}

//! 項から命令を生成する関数
static Obj pTerm()
{
  int opr;
  Obj factor;
  // 式の結果はGR1に格納されている
  if ((factor = pFactor()) == NULL) return NULL;

  while (isMulOp(cur->id)) {
    factor->isLVal = false;

    genCode("PUSH", "0,GR1");
    opr = cur->id;
    consumeToken();
    if (pFactor() == NULL) return NULL;
    genCode("POP", "GR2");
    if (opr == TSTAR) {
      genCode("MULA", "GR1,GR2");
      genCode("JOV", "EOVF");
      factor->type = TPINT;
    } else if (opr == TDIV) {
      genCode("DIVA", "GR2,GR1");
      genCode("JOV", "EOVF");
      genCode("LD", "GR1,GR2");
      factor->type = TPINT;
    } else if (opr == TAND) {
      genCode("AND", "GR1,GR2");
      factor->type = TPBOOL;
    }
  }
  return factor;
}

//! 単純式から命令を生成する関数
static Obj pSimpleExpression()
{
  Obj term;
  if (cur->id == TMINUS) {
    consumeToken();
    // 次の項の値に-1を乗じる
    // 式の結果はスタックに積まれている
    if ((term = pTerm()) == NULL) return NULL;
    genCode("LAD", "GR2,-1");
    genCode("MULA", "GR1,GR2");
    genCode("JOV", "EOVF");
  } else {
    if (cur->id == TPLUS) consumeToken();
    if ((term = pTerm()) == NULL) return NULL;
  }

  while (isAddOp(cur->id)) {
    genCode("PUSH", "0,GR1");
    int opr = cur->id;
    term->isLVal = false;
    consumeToken();
    if ((term = pTerm()) == NULL) return NULL;
    genCode("POP", "GR2");
    if (opr == TPLUS) {
      term->type = TPINT;
      genCode("ADDA", "GR1,GR2");
      genCode("JOV", "EOVF");
    } else if (opr == TMINUS) {
      term->type = TPINT;
      genCode("SUBA", "GR2,GR1");
      genCode("JOV", "EOVF");
      genCode("LD", "GR1,GR2");
    } else if (opr == TOR) {
      term->type = TPBOOL;
      genCode("OR", "GR1,GR2");
    }
  }
  return term;
}

//! 式から命令を生成する関数
static Obj pExpression()
{
  Obj expression;
  int label1, label2;
  // 計算結果はGR1に格納されている
  if ((expression = pSimpleExpression()) == NULL) return NULL;
  while (isRelOp(cur->id)) {
    genCode("PUSH", "0,GR1");
    expression->type = TPBOOL;
    expression->isLVal = false;
    label1 = getLabelNum();
    label2 = getLabelNum();
    int opr = cur->id;
    consumeToken();
    // 計算結果はGR1に格納されている
    pSimpleExpression();
    genCode("POP", "GR2");
    genCode("CPA", "GR2,GR1");

    switch (opr) {
      case TEQUAL:
        genCodeLabel("JZE", label1);
        break;
      case TNOTEQ:
        genCodeLabel("JNZ", label1);
        break;
      case TLE:
        genCodeLabel("JMI", label1);
        break;
      case TLEEQ:
        genCodeLabel("JMI", label1);
        genCodeLabel("JZE", label1);
        break;
      case TGR:
        genCodeLabel("JPL", label1);
        break;
      case TGREQ:
        genCodeLabel("JPL", label1);
        genCodeLabel("JZE", label1);
        break;
      default:
        break;
    }
    genCode("LAD", "GR1,0");
    genCodeLabel("JUMP", label2);
    println("L%04d", label1);
    genCode("LAD", "GR1,1");
    println("L%04d", label2);
  }
  return expression;
}

//! 条件分岐文から命令を生成する関数
static int pCondition()
{
  int label1, label2;
  if (cur->id != TIF) return error("Error at %d: Expected 'if'", cur->line_no);
  consumeToken();
  if (pExpression() == NULL) return ERROR;

  label1 = getLabelNum();
  genCode("CPA", "GR1,GR0");
  println("\tJZE\tL%04d", label1);
  if (cur->id != TTHEN) return error("Error at %d: Expected 'then'", cur->line_no);
  consumeToken();
  at_bol = true;
  if (pStatement() == ERROR) return ERROR;
  if (cur->id == TELSE) {
    label2 = getLabelNum();
    at_bol = true;
    println("\tJUMP\tL%04d", label2);
    genLabel(label1);
    consumeToken();
    if (pStatement() == ERROR) return ERROR;
    genLabel(label2);
  } else {
    genLabel(label1);
  }
  return NORMAL;
}

//! 繰り返し文から命令を生成する関数
static int pIteration()
{
  consumeToken();
  int label1 = getLabelNum();
  int label2 = getLabelNum();
  genLabel(label1);
  pExpression();
  genCode("CPA", "GR1,GR0");
  genCodeLabel("JZE", label2);
  if (cur->id != TDO) return error("Error at %d: Expected 'do'", cur->line_no);
  consumeToken();
  pStatement();
  genCodeLabel("JUMP", label1);
  genLabel(label2);
  return NORMAL;
}

//! 呼び出し文から命令を生成する関数
static void genProcedureCall(Obj obj)
{
  // 左辺値を持つ式なのかどうか
  if (obj->isLVal) {
    // 変数のアドレスをスタックにPUSH
    if (!is_parameter)
      println("\tLAD\tGR1,%s", call_var_name);
    else {
      println("\tLD\tGR1,%s", call_var_name);
    }
    genCode("PUSH", "0,GR1");
  } else {
    // 左辺値を持たない場合
    genCode("LAD", "GR2,=0");
    genCode("ST", "GR1,0,GR2");
    genCode("PUSH", "0,GR2");
  }
}

//! 式の並びから命令を生成する関数
static int pExpressions()
{
  Obj expression;
  needs_address_load = true;
  if ((expression = pExpression()) == NULL) return ERROR;
  genProcedureCall(expression);

  while (cur->id == TCOMMA) {
    consumeToken();
    if ((expression = pExpression()) == NULL) return ERROR;
    genProcedureCall(expression);
  }
  needs_address_load = false;
  return NORMAL;
}

//! 手続き呼び出し文から命令を生成する関数
static int pCall()
{
  if (cur->id != TCALL) return error("Error at %d: Expected 'call'", cur->line_no);
  consumeToken();
  char * procedure_name = getSymbol(cur->str).label;
  consumeToken();
  if (cur->id != TLPAREN) {
    genCode("CALL", procedure_name);
    return NORMAL;
  }
  consumeToken();

  if (pExpressions() == ERROR) return ERROR;

  if (cur->id != TRPAREN) return error("Error at %d: Expected ')'", cur->line_no);
  consumeToken();
  genCode("CALL", procedure_name);
  return NORMAL;
}

//! Read命令で処理できるか型かどうかを判定する関数
static int canReadType(TYPE_KIND type)
{
  switch (type) {
    case TPINT:
    case TPCHAR:
      return true;
    default:
      return false;
  }
}

//! Read命令を生成する関数
static void genRead(TYPE_KIND type)
{
  switch (type) {
    case TPINT:
      genCode("CALL", "READINT");
      break;
    case TPCHAR:
      genCode("CALL", "READCHAR");
      break;
    default:
      error("Error at %d: Expected integer, boolean or char", cur->line_no);
      break;
  }
}

//! 入力文から命令を生成する関数
static int pInput()
{
  bool isReadln = cur->id == TREADLN;
  TYPE_KIND var_type;
  consumeToken();

  if (cur->id != TLPAREN) {
    if (isReadln) genCode("CALL", "READLINE");
    return NORMAL;
  }

  consumeToken();
  needs_address_load = true;
  if ((var_type = pVar()) == TPRERROR) return ERROR;
  needs_address_load = false;
  if (!canReadType(var_type)) return error("Error at %d: Expected integer or char", cur->line_no);
  genRead(var_type);

  while (cur->id == TCOMMA) {
    consumeToken();
    needs_address_load = true;
    if ((var_type = pVar()) == TPRERROR) return ERROR;
    needs_address_load = false;
    if (!canReadType(var_type)) return error("Error at %d: Expected integer or char", cur->line_no);
    genRead(var_type);
  }

  if (cur->id != TRPAREN) return error("Error at %d: Expected ')'", cur->line_no);
  consumeToken();
  if (isReadln) genCode("CALL", "READLINE");

  return NORMAL;
}

//! Write命令を生成する関数
static void genWrite(TYPE_KIND type)
{
  switch (type) {
    case TPINT:
      genCode("CALL", "WRITEINT");
      break;
    case TPBOOL:
      genCode("CALL", "WRITEBOOL");
      break;
    case TPCHAR:
      genCode("CALL", "WRITECHAR");
      break;
    default:
      error("Error at %d: Expected integer, boolean or char", cur->line_no);
      break;
  }
}

//! 出力文のフォーマットから命令を生成する関数
static int pOutputFormat()
{
  if (cur->id == TSTRING && cur->len != 1) {
    println("\tLAD\tGR1,='%s'", cur->str);
    genCode("LAD", "GR2,0");
    genCode("CALL", "WRITESTR");
    consumeToken();
    return NORMAL;
  }

  Obj expression = pExpression();

  int output_num = 0;
  if (cur->id != TCOLON) {
    println("\tLAD\tGR2,%d", output_num);
    genWrite(expression->type);
    return NORMAL;
  }
  consumeToken();
  output_num = cur->num;
  consumeToken();

  println("\tLAD\tGR2,%d", output_num);
  genWrite(expression->type);
  return NORMAL;
}

//! 出力文から命令を生成する関数
static int pOutputStatement()
{
  bool isWriteln = cur->id == TWRITELN;
  consumeToken();
  if (cur->id != TLPAREN) {
    if (isWriteln) genCode("CALL", "WRITELINE");
    return NORMAL;
  }

  consumeToken();
  pOutputFormat();
  while (cur->id == TCOMMA) {
    consumeToken();
    pOutputFormat();
  }

  if (isWriteln) genCode("CALL", "WRITELINE");

  if (cur->id != TRPAREN) return error("Error at %d: Expected ')'", cur->line_no);
  consumeToken();

  return NORMAL;
}

//! 文の構文解析を行う関数
static int pStatement()
{
  switch (cur->id) {
      // 代入文
    case TNAME:
      if (pAssignment() == ERROR) return ERROR;
      break;
    // 分岐文
    case TIF:
      if (pCondition() == ERROR) return ERROR;
      break;
    // 繰り返し文
    case TWHILE:
      if (pIteration() == ERROR) return ERROR;
      break;
    // 脱出文
    case TBREAK:

      consumeToken();
      break;
    // 手続き呼び出し文
    case TCALL:
      if (pCall() == ERROR) return ERROR;
      break;
    // 戻り文
    case TRETURN:
      consumeToken();
      if (procname != NULL) {
        println("\tRET");
      } else {
        genCode("CALL", "FLUSH");
        println("\tSVC\t0");
      }
      break;
    // 入力文
    case TREAD:
    case TREADLN:
      if (pInput() == ERROR) return ERROR;
      break;
    // 出力文
    case TWRITE:
    case TWRITELN:
      if (pOutputStatement() == ERROR) return ERROR;
      break;
    // 複合文
    case TBEGIN:
      if (pCompoundStatement() == ERROR) return ERROR;
      break;

    default:
      // 空文
      break;
  }

  return NORMAL;
}

//! 複合文から命令を生成する関数
static int pCompoundStatement()
{
  if (cur->id != TBEGIN) return error("Error at %d: Expected 'begin'", cur->line_no);
  consumeToken();
  at_bol = true;

  pStatement();
  while (cur->id == TSEMI) {
    consumeToken();
    at_bol = true;
    pStatement();
  }

  if (cur->id != TEND) return error("Error at %d: Expected 'end'", cur->line_no);
  consumeToken();

  return NORMAL;
}

//! 関数の引数を処理する関数
static int pFormalParameters()
{
  if (cur->id != TLPAREN) return error("Error at %d: Expected '('", cur->line_no);
  consumeToken();
  if (cur->kind != TK_IDENT) return error("Error at %d: Expected variable name", cur->line_no);
  pVarNames(true);
  while (cur->id != TSEMI && cur->id != TRPAREN) consumeToken();

  while (cur->id == TSEMI || cur->id == TRPAREN) {
    if (cur->id == TRPAREN) {
      consumeToken();
      break;
    }
  }

  return NORMAL;
}

//! 関数の定義を処理する関数
static int pSubProgram()
{
  procname = cur->str;
  consumeToken();
  if (cur->id == TLPAREN) {
    pFormalParameters();
  }
  consumeToken();
  if (cur->id == TVAR) pVarDeclaration();
  println("%s", getSymbol(procname).label);

  if (!PARAMETER_is_empty(&parameter_stack)) {
    // GR2に戻り番地、GR1に関数の引数を
    genCode("POP", "GR2");
    genCode("POP", "GR1");

    for (;;) {
      char * label = PARAMETER_pop(&parameter_stack);
      println("\tST\tGR1,%s", label);
      if (PARAMETER_is_empty(&parameter_stack)) break;
      genCode("POP", "GR1");
    }
    println("\tPUSH\t0,GR2");
  }

  pCompoundStatement();
  if (cur->id != TSEMI) return error("Error at %d: Expected ';'", cur->line_no);
  consumeToken();
  procname = NULL;
  println("\tRET");
  return NORMAL;
}

//! ブロック文から命令を生成する関数
static int pBlock(char * program_name)
{
  int label = getLabelNum();
  println("%%%%%s\tSTART\tL%04d", program_name, label);

  for (;;) {
    if (cur->id == TVAR) {
      pVarDeclaration();
    } else if (cur->id == TPROCEDURE) {
      consumeToken();
      pSubProgram();
    } else {
      break;
    }
  }
  genLabel(label);
  genCode("LAD", "GR0,0");
  if (pCompoundStatement() == ERROR) return ERROR;

  return NORMAL;
}

//! プログラム文から命令を生成する関数
static int pProgramst()
{
  if (cur->id != TPROGRAM)
    return error("Error at %d: Keyword 'program' is not found", cur->line_no);
  consumeToken();
  char * program_name = cur->str;
  consumeToken();
  consumeToken();

  pBlock(program_name);
  genCode("CALL", "FLUSH");
  println("\tRET");
  consumeToken();

  return NORMAL;
}

//! コード生成を行う関数
int codegen(Token * tok, FILE * output)
{
  output_file = output;
  cur = tok;
  PARAMETER_init(&parameter_stack);

  symbols = parseSymbols(getCrossrefBuf());
  if (pProgramst() == ERROR) return ERROR;
  outlib(output_file);
  println("\tEND");

  return NORMAL;
}
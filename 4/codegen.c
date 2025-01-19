#include <stdbool.h>

#include "lpp.h"
static FILE * output_file;
Token * cur;

//! 定義されたプロシージャの名前を格納する変数
static char * procname = NULL;

//! 副プログラムの仮引数のカウント用変数

DECLARE_STACK(PARAMETER, char *);
PARAMETER parameter_stack;

typedef struct Symbol Symbol;

struct Symbol
{
  char * key;
  char * label;
  char * type;
  char * addr;
};

Symbol * symbols;

static int pCompoundStatement();
static int pStatement();
static TYPE_KIND pExpression();

static Symbol getSymbol(char * key)
{
  for (int i = 0; symbols[i].key != NULL; i++) {
    if (strcmp(symbols[i].key, key) == 0) {
      return symbols[i];
    }
  }
  return (Symbol){NULL, NULL, NULL};
}

static void println(char * fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  vfprintf(output_file, fmt, ap);
  va_end(ap);
  fprintf(output_file, "\n");
}

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
    while (*p != '\n') p++;
    p++;
  }
  return symbols;
}

static void consumeToken() { cur = cur->next; }

static void outlib(void)
{
  fprintf(
    output_file,
    ""
    "; ------------------------\n"
    "; Utility functions\n"
    "; ------------------------\n"
    "EOVF            CALL    WRITELINE\n"
    "                LAD     gr1, EOVF1\n"
    "                LD      gr2, gr0\n"
    "                CALL    WRITESTR\n"
    "                CALL    WRITELINE\n"
    "                SVC     1  ;  overflow error stop\n"
    "EOVF1           DC      '***** Run-Time Error : Overflow *****'\n"
    "E0DIV           JNZ     EOVF\n"
    "                CALL    WRITELINE\n"
    "                LAD     gr1, E0DIV1\n"
    "                LD      gr2, gr0\n"
    "                CALL    WRITESTR\n"
    "                CALL    WRITELINE\n"
    "                SVC     2  ;  0-divide error stop\n"
    "E0DIV1          DC      '***** Run-Time Error : Zero-Divide *****'\n"
    "EROV            CALL    WRITELINE\n"
    "                LAD     gr1, EROV1\n"
    "                LD      gr2, gr0\n"
    "                CALL    WRITESTR\n"
    "                CALL    WRITELINE\n"
    "                SVC     3  ;  range-over error stop\n"
    "EROV1           DC      '***** Run-Time Error : Range-Over in Array Index *****'\n"
    "; gr1の値（文字）をgr2のけた数で出力する．\n"
    "; gr2が0なら必要最小限の桁数で出力する\n"
    "WRITECHAR       RPUSH\n"
    "                LD      gr6, SPACE\n"
    "                LD      gr7, OBUFSIZE\n"
    "WC1             SUBA    gr2, ONE  ; while(--c > 0) {\n"
    "                JZE     WC2\n"
    "                JMI     WC2\n"
    "                ST      gr6, OBUF,gr7  ;  *p++ = ' ';\n"
    "                CALL    BOVFCHECK\n"
    "                JUMP    WC1  ; }\n"
    "WC2             ST      gr1, OBUF,gr7  ; *p++ = gr1;\n"
    "                CALL    BOVFCHECK\n"
    "                ST      gr7, OBUFSIZE\n"
    "                RPOP\n"
    "                RET\n"
    "; gr1が指す文字列をgr2のけた数で出力する．\n"
    "; gr2が0なら必要最小限の桁数で出力する\n"
    "WRITESTR        RPUSH\n"
    "                LD      gr6, gr1  ; p = gr1;\n"
    "WS1             LD      gr4, 0,gr6  ; while(*p != 0) {\n"
    "                JZE     WS2\n"
    "                ADDA    gr6, ONE  ;  p++;\n"
    "                SUBA    gr2, ONE  ;  c--;\n"
    "                JUMP    WS1  ; }\n"
    "WS2             LD      gr7, OBUFSIZE  ; q = OBUFSIZE;\n"
    "                LD      gr5, SPACE\n"
    "WS3             SUBA    gr2, ONE  ; while(--c >= 0) {\n"
    "                JMI     WS4\n"
    "                ST      gr5, OBUF,gr7  ;  *q++ = ' ';\n"
    "                CALL    BOVFCHECK\n"
    "                JUMP    WS3  ; }\n"
    "WS4             LD      gr4, 0,gr1  ; while(*gr1 != 0) {\n"
    "                JZE     WS5\n"
    "                ST      gr4, OBUF,gr7  ;  *q++ = *gr1++;\n"
    "                ADDA    gr1, ONE\n"
    "                CALL    BOVFCHECK\n"
    "                JUMP    WS4  ; }\n"
    "WS5             ST      gr7, OBUFSIZE  ; OBUFSIZE = q;\n"
    "                RPOP\n"
    "                RET\n"
    "BOVFCHECK       ADDA    gr7, ONE\n"
    "                CPA     gr7, BOVFLEVEL\n"
    "                JMI     BOVF1\n"
    "                CALL    WRITELINE\n"
    "                LD      gr7, OBUFSIZE\n"
    "BOVF1           RET\n"
    "BOVFLEVEL       DC      256\n"
    "; gr1の値（整数）をgr2のけた数で出力する．\n"
    "; gr2が0なら必要最小限の桁数で出力する\n"
    "WRITEINT        RPUSH\n"
    "                LD      gr7, gr0  ; flag = 0;\n"
    "                CPA     gr1, gr0  ; if(gr1>=0) goto WI1;\n"
    "                JPL     WI1\n"
    "                JZE     WI1\n"
    "                LD      gr4, gr0  ; gr1= - gr1;\n"
    "                SUBA    gr4, gr1\n"
    "                CPA     gr4, gr1\n"
    "                JZE     WI6\n"
    "                LD      gr1, gr4\n"
    "                LD      gr7, ONE  ; flag = 1;\n"
    "WI1             LD      gr6, SIX  ; p = INTBUF+6;\n"
    "                ST      gr0, INTBUF,gr6  ; *p = 0;\n"
    "                SUBA    gr6, ONE  ; p--;\n"
    "                CPA     gr1, gr0  ; if(gr1 == 0)\n"
    "                JNZ     WI2\n"
    "                LD      gr4, ZERO  ;  *p = '0';\n"
    "                ST      gr4, INTBUF,gr6\n"
    "                JUMP    WI5  ; }\n"
    "; else {\n"
    "WI2             CPA     gr1, gr0  ;  while(gr1 != 0) {\n"
    "                JZE     WI3\n"
    "                LD      gr5, gr1  ;   gr5 = gr1 - (gr1 / 10) * 10;\n"
    "                DIVA    gr1, TEN  ;   gr1 /= 10;\n"
    "                LD      gr4, gr1\n"
    "                MULA    gr4, TEN\n"
    "                SUBA    gr5, gr4\n"
    "                ADDA    gr5, ZERO  ;   gr5 += '0';\n"
    "                ST      gr5, INTBUF,gr6  ;   *p = gr5;\n"
    "                SUBA    gr6, ONE  ;   p--;\n"
    "                JUMP    WI2  ;  }\n"
    "WI3             CPA     gr7, gr0  ;  if(flag != 0) {\n"
    "                JZE     WI4\n"
    "                LD      gr4, MINUS  ;   *p = '-';\n"
    "                ST      gr4, INTBUF,gr6\n"
    "                JUMP    WI5  ;  }\n"
    "WI4             ADDA    gr6, ONE  ;  else p++;\n"
    "; }\n"
    "WI5             LAD     gr1, INTBUF,gr6  ; gr1 = p;\n"
    "                CALL    WRITESTR  ; WRITESTR();\n"
    "                RPOP\n"
    "                RET\n"
    "WI6             LAD     gr1, MMINT\n"
    "                CALL    WRITESTR  ; WRITESTR();\n"
    "                RPOP\n"
    "                RET\n"
    "MMINT           DC      '-32768'\n"
    "; gr1の値（真理値）が0なら'FALSE'を\n"
    "; 0以外なら'TRUE'をgr2のけた数で出力する．\n"
    "; gr2が0なら必要最小限の桁数で出力する\n"
    "WRITEBOOL       RPUSH\n"
    "                CPA     gr1, gr0  ; if(gr1 != 0)\n"
    "                JZE     WB1\n"
    "                LAD     gr1, WBTRUE  ;  gr1 = TRUE;\n"
    "                JUMP    WB2\n"
    "; else\n"
    "WB1             LAD     gr1, WBFALSE  ;  gr1 = FALSE;\n"
    "WB2             CALL    WRITESTR  ; WRITESTR();\n"
    "                RPOP\n"
    "                RET\n"
    "WBTRUE          DC      'TRUE'\n"
    "WBFALSE         DC      'FALSE'\n"
    "; 改行を出力する\n"
    "WRITELINE       RPUSH\n"
    "                LD      gr7, OBUFSIZE\n"
    "                LD      gr6, NEWLINE\n"
    "                ST      gr6, OBUF,gr7\n"
    "                ADDA    gr7, ONE\n"
    "                ST      gr7, OBUFSIZE\n"
    "                OUT     OBUF, OBUFSIZE\n"
    "                ST      gr0, OBUFSIZE\n"
    "                RPOP\n"
    "                RET\n"
    "FLUSH           RPUSH\n"
    "                LD      gr7, OBUFSIZE\n"
    "                JZE     FL1\n"
    "                CALL    WRITELINE\n"
    "FL1             RPOP\n"
    "                RET\n"
    "; gr1が指す番地に文字一つを読み込む\n"
    "READCHAR        RPUSH\n"
    "                LD      gr5, RPBBUF  ; if(RPBBUF != 0) {\n"
    "                JZE     RC0\n"
    "                ST      gr5, 0,gr1  ;  *gr1 = RPBBUF;\n"
    "                ST      gr0, RPBBUF  ;  RPBBUF = 0\n"
    "                JUMP    RC3  ;  return; }\n"
    "RC0             LD      gr7, INP  ; inp = INP;\n"
    "                LD      gr6, IBUFSIZE  ; if(IBUFSIZE == 0) {\n"
    "                JNZ     RC1\n"
    "                IN      IBUF, IBUFSIZE  ;  IN();\n"
    "                LD      gr7, gr0  ;  inp = 0;\n"
    "; }\n"
    "RC1             CPA     gr7, IBUFSIZE  ; if(inp == IBUFSIZE) {\n"
    "                JNZ     RC2\n"
    "                LD      gr5, NEWLINE  ;  *gr1 = '\\n';\n"
    "                ST      gr5, 0,gr1\n"
    "                ST      gr0, IBUFSIZE  ;  IBUFSIZE = INP = 0;\n"
    "                ST      gr0, INP\n"
    "                JUMP    RC3  ; }\n"
    "; else {\n"
    "RC2             LD      gr5, IBUF,gr7  ;  *gr1 = *inp++;\n"
    "                ADDA    gr7, ONE\n"
    "                ST      gr5, 0,gr1\n"
    "                ST      gr7, INP  ;  INP = inp;\n"
    "; }\n"
    "RC3             RPOP\n"
    "                RET\n"
    "; gr1が指す番地に整数値一つを読み込む\n"
    "READINT         RPUSH\n"
    "; do {\n"
    "RI1             CALL    READCHAR  ;  ch = READCHAR();\n"
    "                LD      gr7, 0,gr1\n"
    "                CPA     gr7, SPACE  ; } while(ch==' ' || ch=='\\t' || ch=='\\n');\n"
    "                JZE     RI1\n"
    "                CPA     gr7, TAB\n"
    "                JZE     RI1\n"
    "                CPA     gr7, NEWLINE\n"
    "                JZE     RI1\n"
    "                LD      gr5, ONE  ; flag = 1\n"
    "                CPA     gr7, MINUS  ; if(ch == '-') {\n"
    "                JNZ     RI4\n"
    "                LD      gr5, gr0  ;  flag = 0;\n"
    "                CALL    READCHAR  ;  ch = READCHAR();\n"
    "                LD      gr7, 0,gr1\n"
    "RI4             LD      gr6, gr0  ; v = 0;     ; }\n"
    "RI2             CPA     gr7, ZERO  ; while('0' <= ch && ch <= '9') {\n"
    "                JMI     RI3\n"
    "                CPA     gr7, NINE\n"
    "                JPL     RI3\n"
    "                MULA    gr6, TEN  ;  v = v*10+ch-'0';\n"
    "                ADDA    gr6, gr7\n"
    "                SUBA    gr6, ZERO\n"
    "                CALL    READCHAR  ;  ch = READSCHAR();\n"
    "                LD      gr7, 0,gr1\n"
    "                JUMPgenWriteC      #002D  ; '-'\n"
    "TAB             DC      #0009  ; '\\t'\n"
    "ZERO            DC      #0030  ; '0'\n"
    "NINE            DC      #0039  ; '9'\n"
    "NEWLINE         DC      #000A  ; '\\n'\n"
    "INTBUF          DS      8\n"
    "OBUFSIZE        DC      0\n"
    "IBUFSIZE        DC      0\n"
    "INP             DC      0\n"
    "OBUF            DS      257\n"
    "IBUF            DS      257\n"
    "RPBBUF          DC      0\n");
}

static int getLabelNum()
{
  static int labelcounter = 1;
  return labelcounter++;
}

static void genLabel(int label) { println("L%03d", label); }

static void genCode(char * opc, char * opr) { println("\t%s\t%s", opc, opr); }

static void genCodeLabel(char * opc, int label) { println("\t%s\tL%04d", opc, label); }

static bool isArray(Symbol key) { return key.type[0] == 'a'; }

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
    println("%s\n\tDS\t%d", symbol.label, getArraySize(symbol.type));
  } else {
    if (symbol.label == NULL) {
      return error("Error at %d: Undefined variable %s", cur->line_no, cur->str);
    }
    if (isparam) PARAMETER_push(&parameter_stack, symbol.label);
    println("%s\n\tDC\t0", symbol.label);
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
      println("%s\n\tDS\t%d", symbol.label, getArraySize(symbol.type));
    } else {
      if (symbol.label == NULL) {
        return error("Error at %d: Undefined variable %s", cur->line_no, cur->str);
      }
      if (isparam) PARAMETER_push(&parameter_stack, symbol.label);
      println("%s\n\tDC\t0", symbol.label);
    }
  }
  return NORMAL;
}

static int pVarDeclaration()
{
  consumeToken();
  pVarNames(false);
  while (cur->id != TSEMI) consumeToken();
  consumeToken();

  while (cur->id == TNAME) {
    pVarNames(false);
    while (cur->id != TSEMI) consumeToken();
    consumeToken();
  }

  return NORMAL;
}

static int pVar()
{
  Symbol symbol;
  if (procname != NULL) {
    char key[256];
    snprintf(key, sizeof(key), "%s:%s", cur->str, procname);
    symbol = getSymbol(key);
    if (symbol.label == NULL) symbol = getSymbol(cur->str);
    if (symbol.label == NULL) {
      return error("Error at %d: Undefined variable %s", cur->line_no, cur->str);
    }
  } else {
    symbol = getSymbol(cur->str);
  }
  consumeToken();
  if (cur->id == TLSQPAREN) {
    consumeToken();
    // Expressionの結果はGR1に格納されている
    if (pExpression() == ERROR) return ERROR;
    // 配列の添字が0より大きいかをチェック(GR0には0が常に格納されている)
    genCode("CPA", "GR1,GR0");
    genCode("JMI", "EROV");
    // 配列の添字が配列のサイズより小さいかをチェック
    println("\tLAD\tGR2,%d", getArraySize(symbol.type) - 1);
    genCode("CPA", "GR1,GR2");
    genCode("JPL", "EROV");
    // GR1の分offsetを考慮して配列にアクセスする
    println("\tLAD\tGR1,%s,GR1", symbol.label);
    if (cur->id != TRSQPAREN) return error("Error at %d: Expected ']'", cur->line_no);
    consumeToken();
  } else {
    if (symbol.label == NULL) {
      return error("Error at %d: Undefined variable %s", cur->line_no, cur->str);
    }
    println("\tLD\tGR1,%s", symbol.label);
  }
  return NORMAL;
}

static int pAssignment()
{
  if (pVar() == ERROR) return ERROR;
  genCode("PUSH", "0,GR1");

  if (cur->id != TASSIGN) return error("Error at %d: Expected ':='", cur->line_no);
  consumeToken();

  // Expressionの結果はGR1に格納されている
  if (pExpression() == ERROR) return ERROR;

  // 左辺部の変数のアドレスをスタックからPOP
  genCode("POP", "GR2");
  // GR2には変数のアドレスが格納されているので、そのアドレスにGR1の値を格納する
  genCode("ST", "GR1,0,GR2");

  return NORMAL;
}

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

static TYPE_KIND pFactor()
{
  TYPE_KIND factor_type, expression_type;
  switch (cur->id) {
    // 変数
    case TNAME:
      if ((factor_type = pVar()) == TPRERROR) return TPRERROR;
      break;
    // 定数
    case TNUMBER:
      factor_type = TPINT;
      println("\tLAD\tGR1,%d", cur->num);
      consumeToken();
      break;
    case TFALSE:
      factor_type = TPBOOL;
      println("\tLAD\tGR1,0");
      consumeToken();
      break;
    case TTRUE:
      factor_type = TPBOOL;
      println("\tLAD\tGR1,1");
      consumeToken();
      break;
    case TSTRING:
      factor_type = TPCHAR;
      println("\tLAD\tGR1,%s", cur->str);
      consumeToken();
      break;
      // "(" Expression ")"
    case TLPAREN:
      consumeToken();
      if ((factor_type = pExpression()) == TPRERROR) return TPRERROR;
      if (cur->id != TRPAREN) return error("Error at %d: Expected ')'", cur->line_no);
      consumeToken();
      break;
    case TNOT:
      consumeToken();
      if ((factor_type = pFactor()) == TPRERROR) return TPRERROR;
      genCode("XOR", "GR1,GR1");
      break;
      // 標準型 "(" Expression ")"
    case TINTEGER:
      factor_type = TPINT;
      consumeToken();
      if (cur->id == TLPAREN) return error("Error at %d: Expected '('", cur->line_no);
      consumeToken();
      if ((expression_type = pExpression()) == TPRERROR) return TPRERROR;

      switch (expression_type) {
        case TPINT:
          break;
        case TPBOOL:
          genStoreBoolean();
          break;
        case TPCHAR:
          break;
        default:
          return error("Error at %d: Expected integer", cur->line_no);
      }

      if (cur->id != TRPAREN) return error("Error at %d: Expected ')'", cur->line_no);
      consumeToken();

      break;
    case TBOOLEAN:
      factor_type = TPBOOL;
      consumeToken();
      if (cur->id != TLPAREN) return error("Error at %d: Expected '('", cur->line_no);

      consumeToken();
      if ((expression_type = pExpression()) == TPRERROR) return TPRERROR;

      switch (expression_type) {
        case TPINT:
          genStoreBoolean();
          break;
        case TPBOOL:
          break;
        case TPCHAR:
          genStoreBoolean();
          break;
        default:
          return error("Error at %d: Expected boolean", cur->line_no);
      }
      if (cur->id != TRPAREN) return error("Error at %d: Expected ')'", cur->line_no);
      consumeToken();
      break;
    case TCHAR:
      factor_type = TPCHAR;
      consumeToken();
      if (cur->id != TLPAREN) return error("Error at %d: Expected '('", cur->line_no);
      consumeToken();
      if ((expression_type = pExpression()) == TPRERROR) return TPRERROR;

      switch (expression_type) {
        case TPINT:
          genCode("LAD", "#007F");
          genCode("AND", "GR1,GR2");
          genCode("LAD", "GR2,0");
          break;
        case TPBOOL:
          genStoreBoolean();
          break;
        case TPCHAR:
          break;
        default:
          return error("Error at %d: Expected char", cur->line_no);
      }

      if (cur->id != TRPAREN) return error("Error at %d: Expected ')'", cur->line_no);
      consumeToken();

      break;

    default:
      return error("Error at %d: Expected factor", cur->line_no);
  }
  return factor_type;
}
static TYPE_KIND pTerm()
{
  int opr;
  TYPE_KIND factor_type;
  // 式の結果はGR1に格納されている
  if ((factor_type = pFactor()) == TPRERROR) return TPRERROR;

  while (isMulOp()) {
    genCode("PUSH", "0,GR1");
    opr = cur->id;
    consumeToken();
    if (pFactor() == ERROR) return ERROR;
    genCode("POP", "GR2");
    if (opr == TSTAR) {
      genCode("MULA", "GR1,GR2");
      factor_type = TPINT;
    } else if (opr == TDIV) {
      genCode("DIVA", "GR2,GR1");
      genCode("LD", "GR1,GR2");
      factor_type = TPINT;
    } else if (opr == TAND) {
      genCode("AND", "GR1,GR2");
      factor_type = TPBOOL;
    }
  }
  genCode("JOV", "EOVF");
  return factor_type;
}
static TYPE_KIND pSimpleExpression()
{
  TYPE_KIND term_type;
  if (cur->id == TMINUS) {
    consumeToken();
    // 次の項の値に-1を乗じる
    // 式の結果はスタックに積まれている
    if ((term_type = pTerm()) == TPRERROR) return TPRERROR;
    genCode("LAD", "GR2,1");
    genCode("XOR", "GR1,GR1");
    genCode("ADDA", "GR1,GR2");
  } else {
    if (cur->id == TPLUS) consumeToken();
    if ((term_type = pTerm()) == TPRERROR) return TPRERROR;
  }

  while (isAddOp(cur->id)) {
    genCode("PUSH", "0,GR1");
    int opr = cur->id;
    consumeToken();
    if ((term_type = pTerm()) == TPRERROR) return TPRERROR;
    genCode("POP", "GR2");
    if (opr == TPLUS) {
      term_type = TPINT;
      genCode("ADDA", "GR1,GR2");
    } else if (opr == TMINUS) {
      term_type = TPINT;
      genCode("SUBA", "GR2,GR1");
      genCode("LD", "GR1,GR2");
    } else if (opr == TOR) {
      term_type = TPBOOL;
      genCode("OR", "GR1,GR2");
    }
  }
  return term_type;
}

static TYPE_KIND pExpression()
{
  TYPE_KIND expression_type;
  int label1, label2;
  // 計算結果はGR1に格納されている
  if ((expression_type = pSimpleExpression()) == TPRERROR) return TPRERROR;

  genCode("PUSH", "0,GR1");
  while (isRelOp(cur->id)) {
    expression_type = TPBOOL;
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
  return expression_type;
}

static int pCondition()
{
  int label1, label2;
  if (cur->id != TIF) return error("Error at %d: Expected 'if'", cur->line_no);
  consumeToken();
  if (pExpression() == ERROR) return ERROR;

  label1 = getLabelNum();
  genCode("CPA", "GR1,GR0");
  println("\tJZE\tL%04d", label1);
  if (cur->id != TTHEN) return error("Error at %d: Expected 'then'", cur->line_no);
  consumeToken();
  if (pStatement() == ERROR) return ERROR;
  if (cur->id == TELSE) {
    label2 = getLabelNum();
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
  return NORMAL;
}

static int pCall()
{
  if (cur->id != TCALL) return error("Error at %d: Expected 'call'", cur->line_no);
  consumeToken();
  return NORMAL;
}

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
  if ((var_type = pVar()) == TPRERROR) return ERROR;

  if (!canReadType(var_type)) return error("Error at %d: Expected integer or char", cur->line_no);
  genRead(var_type);
  if (isReadln) genCode("CALL", "READLINE");

  while (cur->id == TCOMMA) {
    consumeToken();
    if ((var_type = pVar()) == TPRERROR) return ERROR;
    if (!canReadType(var_type)) return error("Error at %d: Expected integer or char", cur->line_no);
    genRead(var_type);
    if (isReadln) genCode("CALL", "READLINE");
  }

  if (cur->id != TRPAREN) return error("Error at %d: Expected ')'", cur->line_no);
  consumeToken();

  return NORMAL;
}

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

static int pOutputFormat()
{
  if (cur->id == TSTRING) {
    println("\tLAD\tGR1,='%s'", cur->str);
    genCode("LAD", "GR2,0");
    genCode("CALL", "WRITESTR");
    consumeToken();
    return NORMAL;
  }

  TYPE_KIND type = pExpression();
  int output_num = 0;
  if (cur->id != TCOLON) {
    genWrite(type);
    return NORMAL;
  }
  consumeToken();
  output_num = cur->num;
  consumeToken();

  println("\tLAD\tGR2,%d", output_num);
  genWrite(type);
  return NORMAL;
}

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

static int pCompoundStatement()
{
  if (cur->id != TBEGIN) return error("Error at %d: Expected 'begin'", cur->line_no);
  consumeToken();

  pStatement();
  while (cur->id == TSEMI) {
    consumeToken();
    pStatement();
  }

  if (cur->id != TEND) return error("Error at %d: Expected 'end'", cur->line_no);
  consumeToken();

  println("\tRET");
  return NORMAL;
}

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
    consumeToken();
    pVarNames(true);
    while (cur->id != TSEMI && cur->id != TRPAREN) consumeToken();
  }

  return NORMAL;
}
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
  return NORMAL;
}

static int pBlock()
{
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

  genCode("LAD", "GR0,0");
  if (pCompoundStatement() == ERROR) return ERROR;

  return NORMAL;
}

static int pProgramst()
{
  if (cur->id != TPROGRAM)
    return error("Error at %d: Keyword 'program' is not found", cur->line_no);
  consumeToken();

  println("%%%%%s\tSTART\tL%04d", cur->str, getLabelNum());
  consumeToken();
  consumeToken();

  pBlock();
  consumeToken();

  return NORMAL;
}

int codegen(Token * tok, FILE * output)
{
  output_file = output;
  cur = tok;
  PARAMETER_init(&parameter_stack);

  printf("%s", getCrossrefBuf()->buf);

  symbols = parseSymbols(getCrossrefBuf());
  if (pProgramst() == ERROR) return ERROR;
  outlib();

  return NORMAL;
}
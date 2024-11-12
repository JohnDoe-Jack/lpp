#include <stdio.h>

#include "lpp.h"

/**
 * @file
 * 字句解析を行う為の関数を纏めている
 */

/**
 * @brief キーワードの種類を表す列挙型
 * 
 */
struct KEY
{
  char * keyword;
  int keytoken;
} key[KEYWORDSIZE];

/* keyword list */
struct KEY key[KEYWORDSIZE] = {
  {"and", TAND},       {"array", TARRAY},         {"begin", TBEGIN},     {"boolean", TBOOLEAN},
  {"break", TBREAK},   {"call", TCALL},           {"char", TCHAR},       {"div", TDIV},
  {"do", TDO},         {"else", TELSE},           {"end", TEND},         {"false", TFALSE},
  {"if", TIF},         {"integer", TINTEGER},     {"not", TNOT},         {"of", TOF},
  {"or", TOR},         {"procedure", TPROCEDURE}, {"program", TPROGRAM}, {"read", TREAD},
  {"readln", TREADLN}, {"return", TRETURN},       {"then", TTHEN},       {"true", TTRUE},
  {"var", TVAR},       {"while", TWHILE},         {"write", TWRITE},     {"writeln", TWRITELN}};

/* string of each token */
char * token_str[NUMOFTOKEN + 1] = {
  "",       "NAME",   "program",   "var",     "array",   "of",     "begin",   "end",  "if",
  "then",   "else",   "procedure", "return",  "call",    "while",  "do",      "not",  "or",
  "div",    "and",    "char",      "integer", "boolean", "readln", "writeln", "true", "false",
  "NUMBER", "STRING", "+",         "-",       "*",       "=",      "<>",      "<",    "<=",
  ">",      ">=",     "(",         ")",       "[",       "]",      ":=",      ".",    ",",
  ":",      ";",      "read",      "write",   "break"};

static bool at_bol;

static bool has_space;

/**
 * @brief ひとかたまりの数字トークンを一時的に格納する
 * 
 */
static int num_attr;
/**
 * @brief ひとかたまりの文字列トークンを一時的に格納する
 * 
 */
static char string_attr[MAXSTRSIZE];
/**
 * @brief 解析するファイルを指すポインタ
 * 
 */
FILE * fp;

// /**
//  * @brief ファイルから先読みした1文字を一時的に格納する
//  * 
//  */
// int cbuf;

/**
 * @brief 先読みした時点での行番号を格納する変数
 * 
 */
uint line_num;

/**
 * @brief トークンの行番号を格納する変数
 * 
 */
uint token_line_num;

/**
 * @brief ファイルを開いてエラー出力も行う関数
 * 
 * @param path 
 * @return FILE* 
 */
static FILE * openFile(const char * path)
{
  FILE * out = fopen(path, "r");
  if (!out) error("Cannot open file: %s: %s", path, strerror(errno));
  return out;
}

/**
 * @brief ファイルの読み込みを行う
 * 
 * @param path 
 * @return char* 
 * ファイルの中身を文字列として返す
 */
static char * readFile(char * path)
{
  FILE * fp;
  fp = openFile(path);

  char * buf;
  size_t buflen;
  FILE * out = open_memstream(&buf, &buflen);

  for (;;) {
    char buf2[4096];
    int n = fread(buf2, 1, sizeof(buf2), fp);
    if (n == 0) break;
    fwrite(buf2, 1, n, out);
  }
  fclose(fp);
  fflush(out);
  if (buflen == 0 || buf[buflen - 1] != '\n') fputc('\n', out);
  fputc('\0', out);
  fclose(out);
  return buf;
}

/**
 * @brief keyword listに含まれていかをチェックする
 * 予約語が含まれていた場合、そのトークンの種類を返す
 * @return int
 */
static int checkKeyword()
{
  for (int i = 0; i < KEYWORDSIZE; i++) {
    if (strcmp(string_attr, key[i].keyword) == 0) {
      return key[i].keytoken;
    }
  }
  return TNAME;
}

/**
 * @brief 改行をチェックして、行番号をインクリメントする関数
 * @details \r,\\n,\r\\n,\\n\rの四種類の改行を見分ける
 * @return void
 */
static void checkNewline(char*p)
{
  if (
    (*p == '\r' && *(p++) == '\n') || (*p == '\n' && *(p++) == '\r')) {
    p++;
  }
  line_num++;
}

/**
 * @brief scan()は、ファイルから1文字ずつ読み込んで、トークンを切り出す関数
 * 大域変数cbufには1文字先読みした文字が格納されており、トークンの種類が決定するまでcbufを更新していく
 * @return int 
 * @details トークンの種類を返す
 */
static int scan(char *p)
{
  // トークンの行番号を記録
  token_line_num = line_num;
  for (;;) {
    if (*p == EOF) {
      return S_ERROR;
    }
    switch (*p) {
      // 空白とタブは読み飛ばす
      case ' ':
      case '\t':
        p++;
        continue;
      // 改行文字 (\n または \r)
      case '\n':
      case '\r':
        checkNewline(p);  // 行番号の更新
        continue;
      // {}による注釈を読み飛ばす
      case '{':
        while (*(p++) != '}') {
          if (*p == EOF) {
            error("Expected '}' at end of line (fix available)", getLinenum());
            return S_ERROR;
          }
        }
        p++;
        continue;
      // /* */による注釈を読み飛ばす
      case '/':
        p++;
        if (*p == '*') {
          // */が出るまで読み飛ばす
          while (1) {
            while (*(p++) != '*') {
              if (*p == EOF) return S_ERROR;
            }
            if (*(p++) == '/') {
              break;
            }
          }
          p++;
          continue;
        } else {
          fprintf(stderr, "Illegal character: %c\n at %d line.", *p, getLinenum());
          return S_ERROR;
        }
    }

    if (isalpha(*p)) {
      // 名前の読み込み
      int i = 0;
      do {
        string_attr[i++] = *p;
        if (i < MAXSTRSIZE - 1)
          string_attr[i] = '\0';
        else {
          fprintf(stderr, "Too long string at %d line.", getLinenum());
          return S_ERROR;
        }
        *p = fgetc(fp);
      } while (isalnum(*p));
      return checkKeyword();
    } else if (isdigit(*p)) {
      // 数字の読み込み
      num_attr = 0;
      do {
        num_attr = num_attr * 10 + (*p - '0');
        if (num_attr > MAXNUM) {
          error("Too big number.", getLinenum());
          return S_ERROR;
        }
        *p = fgetc(fp);
      } while (isdigit(*p));
      return TNUMBER;
    } else if (*p == '\'') {
      // 'で囲まれた文字列の読み込み
      int i = 0;
      *p = fgetc(fp);  // 最初の'を読み飛ばす
      for (;;) {
        if (i >= MAXSTRSIZE - 1) return S_ERROR;
        if (*p == EOF) return S_ERROR;  // 'で閉じる前にEOFになった場合
        if (*p == '\'') {
          *p = fgetc(fp);
          if (*p != '\'') break;
        }
        string_attr[i++] = *p;
        *p = fgetc(fp);
      }

      if (*p == EOF) return S_ERROR;  // 'で閉じる前にEOFになった場合
      string_attr[i] = '\0';
      return TSTRING;
    } else {
      // 1文字のトークン
      switch (*p) {
        case '+':
          p++;
          return TPLUS;
        case '-':
          p++;
          return TMINUS;
        case '*':
          p++;
          return TSTAR;
        case '=':
          p++;
          return TEQUAL;
        case '(':
         p++;
          return TLPAREN;
        case ')':
           p++;
          return TRPAREN;
        case '[':
           p++;
          return TLSQPAREN;
        case ']':
           p++;
          return TRSQPAREN;
        case '.':
          p++;
          return TDOT;
        case ',':
           p++;
          return TCOMMA;
        case ';':
           p++;
          return TSEMI;
        case '<':
          p++;
          if (*p == '=') {
            p++;
            return TLEEQ;
          } else if (*p == '>') {
            p++;
            return TNOTEQ;
          } else {
            return TLE;
          }
        case '>':
          p++;
          if (*p == '=') {
             p++;
            return TGREQ;
          } else {
            return TGR;
          }

        case ':':
           p++;
          if (*p == '=') {
            p++;
            return TASSIGN;
          } else {
            return TCOLON;
          }
        default:
          printf("Illegal character: %c\n", *p);
          printf("line: %d\n", line_num);
          p++;
          return S_ERROR;
      }
    }
  }
}

static Token *newToken(TokenKind kind,char *start){
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->loc = start;
  tok->at_bol = at_bol;
  tok->has_space = has_space;
  tok->line_no = token_line_num;

  at_bol = has_space = false;

  return tok;
}

Token *tokenize(char *p){
  at_bol = true;
  has_space = false;

  while(*p){
      scan(p);
  }
  return newToken(TK_EOF, NULL);
}

/**
 * @brief トークンのリストを返す関数
 * 入力されたファイルに対して構文解析を行い、tokenize関数でトークンのリストを返す
 * @param path 
 * @return Token* 
 */
Token * tokenizeFile(char * path)
{
  char * p = readFile(path);

  if (!p) return NULL;
  return tokenize(p);
}

/**
 * @brief 行番号を返す関数
 * token_line_numは、1個のトークンが確定した時点での行番号を示す
 * @return int 
 */
int getLinenum() { return token_line_num; }

/**
 * @brief 字句解析が終わったあとに呼び出される関数
 * fpは解析するファイルを指すポインタで、解析が終わったら閉じる
 */
void endScan() { fclose(fp); }
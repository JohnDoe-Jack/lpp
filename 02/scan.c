#include "lpp.h"

/**
 * @file
 * 字句解析を行う為の関数を纏めている
 */

#define NUMOFPUNCT 18

/**
 * @brief キーワードの種類を表す列挙型
 * 
 */
static struct KEY
{
  char * keyword;
  int keytoken;
} key[KEYWORDSIZE];

/**
 * @brief 予約語のリスト
 * 
 */
static struct KEY key[KEYWORDSIZE] = {
  {"and", TAND},       {"array", TARRAY},         {"begin", TBEGIN},     {"boolean", TBOOLEAN},
  {"break", TBREAK},   {"call", TCALL},           {"char", TCHAR},       {"div", TDIV},
  {"do", TDO},         {"else", TELSE},           {"end", TEND},         {"false", TFALSE},
  {"if", TIF},         {"integer", TINTEGER},     {"not", TNOT},         {"of", TOF},
  {"or", TOR},         {"procedure", TPROCEDURE}, {"program", TPROGRAM}, {"read", TREAD},
  {"readln", TREADLN}, {"return", TRETURN},       {"then", TTHEN},       {"true", TTRUE},
  {"var", TVAR},       {"while", TWHILE},         {"write", TWRITE},     {"writeln", TWRITELN}};

/**
 * @brief 区切り文字のリスト
 * 
 */

static struct KEY punct[PUNCTSIZE] = {
  {":=", TASSIGN},  {"<>", TNOTEQ},   {">=", TGREQ}, {"<=", TLEEQ}, {"+", TPLUS},   {"-", TMINUS},
  {"*", TSTAR},     {"=", TEQUAL},    {"<", TLE},    {">", TGR},    {"(", TLPAREN}, {")", TRPAREN},
  {"[", TLSQPAREN}, {"]", TRSQPAREN}, {".", TDOT},   {",", TCOMMA}, {":", TCOLON},  {";", TSEMI}};

/**
 * @brief 現在のトークンの位置が行頭かどうかを表す変数
 * 
 */
static bool at_bol;

/**
 * @brief 現在のトークンに空白が続くかを表す変数
 * 
 */
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

// /**
//  * @brief ファイルから先読みした1文字を一時的に格納する
//  *
//  */
// int cbuf;

/**
 * @brief 先読みした時点での行番号を格納する変数
 * 
 */
uint line_num = 1;

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
 * @brief 新しいトークンを生成する関数
 * 
 * @param kind トークンの種類
 * @param id トークン識別ID
 * @param len トークンの長さ
 * @return Token* 
 */
static Token * newToken(TokenKind kind, int id, int len)
{
  Token * tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->id = id;
  tok->len = len;
  tok->at_bol = at_bol;
  tok->has_space = has_space;
  tok->line_no = line_num;

  at_bol = has_space = false;

  return tok;
}

/**
 * @brief keyword listに含まれていかをチェックする
 * 予約語が含まれていた場合、その予約語トークンを返す。もし含まれていなければ識別子トークンを返す
 * @return int
 */
static Token * checkKeyword(Token * cur)
{
  for (int i = 0; i < KEYWORDSIZE; i++) {
    if (strcmp(string_attr, key[i].keyword) == 0) {
      cur = cur->next = newToken(TK_KEYWORD, key[i].keytoken, strlen(key[i].keyword));
      return cur;
    }
  }
  cur = cur->next = newToken(TK_IDENT, TNAME, strlen(string_attr));
  cur->str = strdup(string_attr);
  return cur;
}

/**
 * @brief 句読点が含まれているかをチェックする
 * 句読点が含まれていた場合、そのトークンのIDを返す。もし含まれていなければ-1を返す
 * @param p 
 * @return int 
 */
static int checkPunct(char * p)
{
  for (int i = 0; i < PUNCTSIZE; i++) {
    if (strncmp(p, punct[i].keyword, strlen(punct[i].keyword)) == 0) {
      return punct[i].keytoken;
    }
  }
  return -1;
}

/**
 * @brief トークナイズする対象の文字列に字句解析を行い、トークンのリストを返す関数
 * 
 * @param p ファイル内の1文字を指すポインタ
 * @param head トークンのリストの先頭
 * @return Token* 
 */
static Token * scan(char * p, Token * head)
{
  Token * cur = head;
  at_bol = true;
  has_space = false;
  for (;;) {
    if (*p == '\0') {
      cur = cur->next = newToken(TK_EOF, 0, 0);
      return head->next;
    }
    switch (*p) {
      // 空白とタブは読み飛ばす
      case ' ':
      case '\t':
        p++;
        has_space = true;
        continue;
      // 改行文字 (\n または \r)
      case '\n':
      case '\r':
        if ((*p == '\r' && *p++ == '\n') || (*p == '\n' && *p++ == '\r')) {
          p++;
        }
        line_num++;
        at_bol = true;
        has_space = false;
        continue;
      // {}による注釈を読み飛ばす
      case '{':
        while (*(p++) != '}') {
          if (*p == '\0') {
            error("Expected '}' at end of line (fix available)", line_num);
            cur = cur->next = newToken(TK_EOF, 0, 0);
            return head->next;
          }
        }
        p++;
        has_space = true;
        continue;
      // /* */による注釈を読み飛ばす
      // TODO: 関数化したい
      case '/':
        p++;
        if (*p == '*') {
          // */が出るまで読み飛ばす
          while (1) {
            while (*(p++) != '*') {
              if (*p == '\0') {
                cur = cur->next = newToken(TK_EOF, 0, 0);
                return head->next;
              }
            }
            if (*(p++) == '/') {
              break;
            }
          }
          p++;
          has_space = true;
          continue;
        } else {
          fprintf(stderr, "Illegal character: %c\n at %d line.", *p, line_num);
          cur = cur->next = newToken(TK_EOF, 0, 0);
          return head->next;
        }
    }

    if (isalpha(*p)) {
      // 名前の読み込み
      // TODO: 関数化したい
      int i = 0;
      do {
        string_attr[i++] = *p;
        if (i < MAXSTRSIZE - 1)
          string_attr[i] = '\0';
        else {
          error("Too long string at %d line.", line_num);
          cur = cur->next = newToken(TK_EOF, 0, 0);
          return head->next;
        }
        p++;
      } while (isalnum(*p));
      cur = checkKeyword(cur);
      int id = cur->id;
      if (id == TPROCEDURE || id == TVAR || id == TBEGIN || id == TEND || id == TELSE || id == TDO)
        cur->at_bol = true;
      continue;
    } else if (isdigit(*p)) {
      // 数字の読み込み
      // TODO: 関数化したい
      int len = 0;
      num_attr = 0;
      do {
        len++;
        num_attr = num_attr * 10 + (*p - '0');
        if (num_attr > MAXNUM) {
          error("Too big number.", line_num);
          cur = cur->next = newToken(TK_EOF, 0, 0);
          return head->next;
        }
        p++;
      } while (isdigit(*p));
      cur = cur->next = newToken(TK_NUM, TNUMBER, len);
      cur->num = num_attr;
    } else if (*p == '\'') {
      // 'で囲まれた文字列の読み込み
      // TODO: 関数化したい
      int str_len = 0;
      p++;  // 最初の'を読み飛ばす
      for (;;) {
        if (str_len >= MAXSTRSIZE - 1) {
          error("Too long string at %d line.", line_num);
          cur = cur->next = newToken(TK_EOF, 0, 0);
          return head->next;
        }
        if (*p == '\0') {
          cur = cur->next = newToken(TK_EOF, 0, 0);  // 'で閉じる前にEOFになった場合
          return head->next;
        }
        if (*p == '\'') {
          p++;
          if (*p != '\'') break;
        }
        string_attr[str_len++] = *p;
        p++;
      }

      if (*p == '\0') {
        cur = cur->next = newToken(TK_EOF, 0, 0);  // 'で閉じる前にEOFになった場合
        return head->next;
      }
      string_attr[str_len] = '\0';
      cur = cur->next = newToken(TK_STR, TSTRING, str_len);
      cur->str = strdup(string_attr);
    } else {
      // その他の記号
      // TODO: 関数化したい
      int punct_id = checkPunct(p);
      if (punct_id != -1) {
        const char * token_str[NUMOFPUNCT + 1] = {"",   "+", "-",  "*", "=", "<>", "<",
                                                  "<=", ">", ">=", "(", ")", "[",  "]",
                                                  ":=", ".", ",",  ":", ";"};
        cur = cur->next = newToken(TK_PUNCT, punct_id, strlen(token_str[punct_id - 28]));
        if (cur->id == TSEMI || cur->id == TDOT || cur->id == TCOMMA) cur->has_space = false;
        p += strlen(token_str[punct_id - 28]);
      } else {
        fprintf(stderr, "Illegal character: %c\n at %d line.", *p, line_num);
        cur = cur->next = newToken(TK_EOF, 0, 0);
        return head->next;
      }
    }
  }
  // Unreachable
  return head->next;
}

/**
 * @brief トークンのリストを返す関数
 * トークンのリストは単方向の連結リストで表現される
 * @param p 
 * @return Token* 
 */
Token * tokenize(char * p)
{
  Token head = {};
  head.next = scan(p, &head);
  return head.next;
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

int get_linenum() { return line_num; }
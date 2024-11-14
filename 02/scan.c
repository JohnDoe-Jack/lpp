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
 * 予約語が含まれていた場合、そのトークンの種類を返す
 * @return int
 */
static void checkKeyword(Token * cur)
{
  // printf("string_attr: %s\n", string_attr);
  for (int i = 0; i < KEYWORDSIZE; i++) {
    if (strcmp(string_attr, key[i].keyword) == 0) {
      cur = cur->next = newToken(TK_KEYWORD, key[i].keytoken, strlen(key[i].keyword));
    }
  }
  printf("string_attr: %s\n", string_attr);
  cur = cur->next = newToken(TK_IDENT, TNAME, strlen(string_attr));
}

/**
 * @brief scan()は、ファイルから1文字ずつ読み込んで、トークンを切り出す関数
 * pには1文字先読みした文字が格納されており、トークンの種類が決定するまでcbufを更新していく
 * @return Token* 
 * @details トークンの種類を返す
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
        continue;
      // 改行文字 (\n または \r)
      case '\n':
      case '\r':
        if ((*p == '\r' && *p++ == '\n') || (*p == '\n' && *p++ == '\r')) {
          p++;
        }
        printf("line_num: %d\n", line_num);
        line_num++;
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
        continue;
      // /* */による注釈を読み飛ばす
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
          continue;
        } else {
          fprintf(stderr, "Illegal character: %c\n at %d line.", *p, line_num);
          cur = cur->next = newToken(TK_EOF, 0, 0);
          return head->next;
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
          fprintf(stderr, "Too long string at %d line.", line_num);
          cur = cur->next = newToken(TK_EOF, 0, 0);
          return head->next;
        }
        p++;
      } while (isalnum(*p));
      checkKeyword(cur);
      continue;
    } else if (isdigit(*p)) {
      // 数字の読み込み
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
      cur->str = string_attr;
      printf("string_attr: %s\n", string_attr);
    } else {
      // 1文字のトークン
      switch (*p) {
        case '+':
          p++;
          cur = cur->next = newToken(TK_PUNCT, TPLUS, 1);
          break;
        case '-':
          p++;
          cur = cur->next = newToken(TK_PUNCT, TMINUS, 1);
          break;
        case '*':
          p++;
          cur = cur->next = newToken(TK_PUNCT, TSTAR, 1);
          break;
        case '=':
          p++;
          cur = cur->next = newToken(TK_PUNCT, TEQUAL, 1);
          break;
        case '(':
          p++;
          cur = cur->next = newToken(TK_PUNCT, TLPAREN, 1);
          break;
        case ')':
          p++;
          cur = cur->next = newToken(TK_PUNCT, TRPAREN, 1);
          break;
        case '[':
          p++;
          cur = cur->next = newToken(TK_PUNCT, TLSQPAREN, 1);
          break;
        case ']':
          p++;
          cur = cur->next = newToken(TK_PUNCT, TRSQPAREN, 1);
          break;
        case '.':
          p++;
          cur = cur->next = newToken(TK_PUNCT, TDOT, 1);
          break;
        case ',':
          p++;
          cur = cur->next = newToken(TK_PUNCT, TCOMMA, 1);
          break;
        case ';':
          p++;
          cur = cur->next = newToken(TK_PUNCT, TSEMI, 1);
          break;
        case '<':
          p++;
          if (*p == '=') {
            p++;
            cur = cur->next = newToken(TK_PUNCT, TLEEQ, 2);
            break;
          } else if (*p == '>') {
            p++;
            cur = cur->next = newToken(TK_PUNCT, TNOTEQ, 2);
            break;
          } else {
            cur = cur->next = newToken(TK_PUNCT, TLE, 1);
            break;
          }
        case '>':
          p++;
          if (*p == '=') {
            p++;
            cur = cur->next = newToken(TK_PUNCT, TGREQ, 2);
            break;
          } else {
            cur = cur->next = newToken(TK_PUNCT, TGR, 1);
            break;
          }

        case ':':
          p++;
          if (*p == '=') {
            p++;
            cur = cur->next = newToken(TK_PUNCT, TASSIGN, 2);
            break;
          } else {
            cur = cur->next = newToken(TK_PUNCT, TCOLON, 1);
            break;
          }
        default:
          printf("Illegal character: %c\n", *p);
          printf("line: %d\n", line_num);
          p++;
          cur = cur->next = newToken(TK_EOF, 0, 0);
          return head->next;
      }
    }
  }
  // Unreachable
  return head->next;
}

Token * tokenize(char * p)
{
  Token head = {};
  head.next = scan(p, &head);
  printf("head.next->kind: %d\n", head.next->kind);
  printf("head.next->next->kind: %d\n", head.next->next->kind);
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
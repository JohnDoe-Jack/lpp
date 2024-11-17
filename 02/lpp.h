#ifndef LPP_H
#define LPP_H
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define MAXSTRSIZE 1024
#define MAXNUM 32768

/**
 * @brief トークンのID
 * IDは1ベースである
 */
typedef enum {
  TNAME = 1,
  TPROGRAM,
  TVAR,
  TARRAY,
  TOF,
  TBEGIN,
  TEND,
  TIF,
  TTHEN,
  TELSE,
  TPROCEDURE,
  TRETURN,
  TCALL,
  TWHILE,
  TDO,
  TNOT,
  TOR,
  TDIV,
  TAND,
  TCHAR,
  TINTEGER,
  TBOOLEAN,
  TREADLN,
  TWRITELN,
  TTRUE,
  TFALSE,
  TNUMBER,
  TSTRING,
  TPLUS,
  TMINUS,
  TSTAR,
  TEQUAL,
  TNOTEQ,
  TLE,
  TLEEQ,
  TGR,
  TGREQ,
  TLPAREN,
  TRPAREN,
  TLSQPAREN,
  TRSQPAREN,
  TASSIGN,
  TDOT,
  TCOMMA,
  TCOLON,
  TSEMI,
  TREAD,
  TWRITE,
  TBREAK
} TokenID;

#define NUMOFTOKEN 49

#define KEYWORDSIZE 28
#define PUNCTSIZE 18
#define S_ERROR -1

#define ERROR 1
#define NORMAL 0

/**
 * @brief トークンの種類を表す列挙型
 * 
 */
typedef enum {
  TK_IDENT,    // 識別子
  TK_PUNCT,    // 区切り記号
  TK_KEYWORD,  // キーワード
  TK_STR,      // 文字列
  TK_NUM,      // 数値
  TK_EOF,      // ファイルの終わり
} TokenKind;

typedef struct Token Token;

/**
 * @brief トークンの構造体
 * TokenKind kind トークンの種類
 * int id トークンのID
 * Token * next 次のトークン
 * char *loc トークンの位置
 * int len トークンの長さ
 * int line_no トークンの行
 * char * str トークン文字列
 * int num 数値
 * bool at_bol 行頭かどうか
 * bool has_space トークンの前に空白があるか
 */
struct Token
{
  TokenKind kind;  // トークンの種類
  int id;          // トークンのID
  Token * next;    // 次のトークン
  int len;         // トークンの長さ
  int line_no;     // トークンの行
  char * str;      // トークン文字列
  int num;         // 数値
  bool at_bol;     // 行頭かどうか
  bool has_space;  // トークンの前に空白があるか
};

int error(char *, ...);
Token * tokenizeFile(char *);
int get_linenum();
void parse(Token *);
#endif
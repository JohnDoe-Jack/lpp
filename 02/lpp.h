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

/**
 * @brief 文字列の最大の長さを表す定数
 * @def MAXSTRSIZE
 */
#define MAXSTRSIZE 1024

/**
 * @brief 数値の最大の長さを表す定数
 * @def MAXNUM
 */
#define MAXNUM 32768

/**
 * @brief トークンのID。
 * IDは1から始まる。define定数で実装されていたものを列挙型に変更した。
 */
typedef enum {
  //! 名前
  TNAME = 1,
  //! プログラム
  TPROGRAM,
  //! 変数
  TVAR,
  //! 配列
  TARRAY,
  //! of
  TOF,
  //! begin
  TBEGIN,
  //! end
  TEND,
  //! if
  TIF,
  //! then
  TTHEN,
  //! else
  TELSE,
  //! procedure
  TPROCEDURE,
  //! return
  TRETURN,
  //! call
  TCALL,
  //! while
  TWHILE,
  //! do
  TDO,
  //! not
  TNOT,
  //! or
  TOR,
  //! div
  TDIV,
  //! and
  TAND,
  //! char
  TCHAR,
  //! integer
  TINTEGER,
  //! boolean
  TBOOLEAN,
  //! readln
  TREADLN,
  //! writeln
  TWRITELN,
  //! true
  TTRUE,
  //! false
  TFALSE,
  //! 数値
  TNUMBER,
  //! 文字列
  TSTRING,
  //! 加算
  TPLUS,
  //! 減算
  TMINUS,
  //! 乗算
  TSTAR,
  //! 除算
  TEQUAL,
  //! 不等号
  TNOTEQ,
  //! 小なり
  TLE,
  //! 小なりイコール
  TLEEQ,
  //! 大なり
  TGR,
  //! 大なりイコール
  TGREQ,
  //! 左括弧
  TLPAREN,
  //! 右括弧
  TRPAREN,
  //! 左角括弧
  TLSQPAREN,
  //! 右角括弧
  TRSQPAREN,
  //! 代入
  TASSIGN,
  //! ドット
  TDOT,
  //! カンマ
  TCOMMA,
  //! コロン
  TCOLON,
  //! セミコロン
  TSEMI,
  //! read
  TREAD,
  //! write
  TWRITE,
  //! break
  TBREAK
} TokenID;

/**
 * @def NUMOFTOKEN
 * トークンの種類の数
 */
#define NUMOFTOKEN 49

/**
 * @def KEYWORDSIZE
 * キーワードの数
 */
#define KEYWORDSIZE 28

/**
 * @def PUNCTSIZE
 * 区切り記号の数
 */
#define PUNCTSIZE 18

/**
 * @def
 * エラーを示す定数
 */
#define S_ERROR -1

/**
 * @def ERROR
 * @brief 解析途中で失敗したことを示す定数
 */
#define ERROR 1

/**
 * @def NORMAL
 * @brief 正常終了を示す定数
 */
#define NORMAL 0

/**
 * @enum TokenKind
 * @brief brief description
 */
typedef enum {
  //! 識別子
  TK_IDENT,
  //! 区切り記号
  TK_PUNCT,
  //! キーワード
  TK_KEYWORD,
  //! 文字列
  TK_STR,
  //! 数値
  TK_NUM,
  //! ファイルの終わり
  TK_EOF,
} TokenKind;

/**
 * @struct Token
 * @brief トークンの構造体
 */
typedef struct Token Token;

/**
 * @struct Token
 * @brief トークンの構造体
 */
struct Token
{
  //! トークンの種類
  TokenKind kind;
  //! トークンのID
  int id;
  //! 次のトークン
  Token * next;
  //! トークンの長さ
  int len;
  //! トークンの行
  int line_no;
  //! トークンの文字列
  char * str;
  //! トークンの数値
  int num;
  //! 行頭かどうか
  bool at_bol;
  //! トークンの前に空白があるか
  bool has_space;
};

int error(char *, ...);
Token * tokenizeFile(char *);
void parse(Token *);
#endif
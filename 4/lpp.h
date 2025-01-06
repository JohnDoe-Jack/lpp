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
  //! 等号
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
  TBREAK,
  //! エラー
  TERROR
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
#define ERROR -1

/**
 * @def NORMAL
 * @brief 正常終了を示す定数
 */
#define NORMAL 0

#define HASHSIZE 1000

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

typedef enum {
  //! 整数
  TPINT,
  //! 文字
  TPCHAR,
  //! 真偽値
  TPBOOL,
  //! 整数型の配列
  TPARRAYINT,
  //! 文字型の配列
  TPARRAYCHAR,
  //! 真偽値の配列
  TPARRAYBOOL,
  //! 手続き
  TPPROC,
  //! エラー
  TPRERROR
} TYPE_KIND;

typedef struct TYPE TYPE;
struct TYPE
{
  //! 型の種類
  TYPE_KIND ttype;
  //! 配列型の場合の配列サイズ
  int arraysize;
  //! 配列型の場合の要素の型
  TYPE * etp;
  //! 手続き型の場合の仮引数の型リスト
  TYPE * paratp;
};

typedef struct LINE LINE;
struct LINE
{
  int reflinenum;
  LINE * nextlinep;
};

typedef struct ID ID;
struct ID
{
  char * name;
  char * procname;
  TYPE * itp;
  bool ispara;
  int defline;
  LINE * irefp;
};

typedef struct Entry Entry;
struct Entry
{
  char * key;
  ID * value;
  Entry * next;
};

typedef struct HashMap HashMap;
struct HashMap
{
  Entry ** entries;
  int size;
};

HashMap * newHashMap(int);
void insertToHashMap(const HashMap *, const char *, ID *);
ID * getValueFromHashMap(const HashMap *, const char *);
void freeHashMap(HashMap *);
int removeFromHashMap(const HashMap *, const char *);

/**
 * @brief 独自型を含めた任意の型に対応するためのStackマクロ
 * @def DECLARE_STACK
 */
#define DECLARE_STACK(TYPENAME, ELEMENT_TYPE)                                                     \
  typedef struct                                                                                  \
  {                                                                                               \
    size_t capacity;     /* スタックの容量 */                                              \
    size_t size;         /* スタックの現在のサイズ */                                  \
    ELEMENT_TYPE * data; /* スタックのデータ領域 */                                     \
  } TYPENAME;                                                                                     \
                                                                                                  \
  /* スタックの初期化 */                                                                  \
  static void TYPENAME##_init(TYPENAME * stack)                                                   \
  {                                                                                               \
    stack->capacity = 16;                                                                         \
    stack->size = 0;                                                                              \
    stack->data = (ELEMENT_TYPE *)malloc(sizeof(ELEMENT_TYPE) * stack->capacity);                 \
    if (!stack->data) {                                                                           \
      fprintf(stderr, "Failed to allocate memory for stack\n");                                   \
      exit(EXIT_FAILURE);                                                                         \
    }                                                                                             \
  }                                                                                               \
                                                                                                  \
  /* スタックに値をプッシュ */                                                         \
  static void TYPENAME##_push(TYPENAME * stack, ELEMENT_TYPE value)                               \
  {                                                                                               \
    if (stack->size >= stack->capacity) {                                                         \
      stack->capacity *= 2;                                                                       \
      stack->data = (ELEMENT_TYPE *)realloc(stack->data, sizeof(ELEMENT_TYPE) * stack->capacity); \
      if (!stack->data) {                                                                         \
        fprintf(stderr, "Failed to reallocate memory for stack\n");                               \
        exit(EXIT_FAILURE);                                                                       \
      }                                                                                           \
    }                                                                                             \
    stack->data[stack->size++] = value;                                                           \
  }                                                                                               \
                                                                                                  \
  /* スタックから値をポップ */                                                         \
  static ELEMENT_TYPE TYPENAME##_pop(TYPENAME * stack)                                            \
  {                                                                                               \
    if (stack->size == 0) {                                                                       \
      fprintf(stderr, "Stack underflow\n");                                                       \
      exit(EXIT_FAILURE);                                                                         \
    }                                                                                             \
    return stack->data[--stack->size];                                                            \
  }                                                                                               \
                                                                                                  \
  /* スタックのトップを参照（ポップしない） */                                 \
  static ELEMENT_TYPE TYPENAME##_peek(TYPENAME * stack)                                           \
  {                                                                                               \
    if (stack->size == 0) {                                                                       \
      fprintf(stderr, "Stack is empty\n");                                                        \
      exit(EXIT_FAILURE);                                                                         \
    }                                                                                             \
    return stack->data[stack->size - 1];                                                          \
  }                                                                                               \
                                                                                                  \
  /* スタックが空かどうかを判定 */                                                   \
  static int TYPENAME##_is_empty(TYPENAME * stack) { return stack->size == 0; }                   \
                                                                                                  \
  /* スタックのサイズを取得 */                                                         \
  static size_t TYPENAME##_size(TYPENAME * stack) { return stack->size; }                         \
                                                                                                  \
  /* スタックの解放 */                                                                     \
  static void TYPENAME##_destroy(TYPENAME * stack)                                                \
  {                                                                                               \
    free(stack->data);                                                                            \
    stack->data = NULL;                                                                           \
    stack->capacity = stack->size = 0;                                                            \
  }

TYPE_KIND error(char *, ...);

Token * tokenizeFile(char *);
void parse(Token *);

void outlib(void);
void codegen(Token *);
#endif
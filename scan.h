#ifndef SCAN_H
#define SCAN_H

/* scan.h  */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXSTRSIZE 1024
#define MAXNUM 32768

/* Token */
#define TNAME 1       /* Name : Alphabet { Alphabet | Digit } */
#define TPROGRAM 2    /* program : Keyword */
#define TVAR 3        /* var : Keyword */
#define TARRAY 4      /* array : Keyword */
#define TOF 5         /* of : Keyword */
#define TBEGIN 6      /* begin : Keyword */
#define TEND 7        /* end : Keyword */
#define TIF 8         /* if : Keyword */
#define TTHEN 9       /* then : Keyword */
#define TELSE 10      /* else : Keyword */
#define TPROCEDURE 11 /* procedure : Keyword */
#define TRETURN 12    /* return : Keyword */
#define TCALL 13      /* call : Keyword */
#define TWHILE 14     /* while : Keyword */
#define TDO 15        /* do : Keyword */
#define TNOT 16       /* not : Keyword */
#define TOR 17        /* or : Keyword */
#define TDIV 18       /* div : Keyword */
#define TAND 19       /* and : Keyword */
#define TCHAR 20      /* char : Keyword */
#define TINTEGER 21   /* integer : Keyword */
#define TBOOLEAN 22   /* boolean : Keyword */
#define TREADLN 23    /* readln : Keyword */
#define TWRITELN 24   /* writeln : Keyword */
#define TTRUE 25      /* true : Keyword */
#define TFALSE 26     /* false : Keyword */
#define TNUMBER 27    /* unsigned integer */
#define TSTRING 28    /* String */
#define TPLUS 29      /* + : symbol */
#define TMINUS 30     /* - : symbol */
#define TSTAR 31      /* * : symbol */
#define TEQUAL 32     /* = : symbol */
#define TNOTEQ 33     /* <> : symbol */
#define TLE 34        /* < : symbol */
#define TLEEQ 35      /* <= : symbol */
#define TGR 36        /* > : symbol */
#define TGREQ 37      /* >= : symbol */
#define TLPAREN 38    /* ( : symbol */
#define TRPAREN 39    /* ) : symbol */
#define TLSQPAREN 40  /* [ : symbol */
#define TRSQPAREN 41  /* ] : symbol */
#define TASSIGN 42    /* := : symbol */
#define TDOT 43       /* . : symbol */
#define TCOMMA 44     /* , : symbol */
#define TCOLON 45     /* : : symbol */
#define TSEMI 46      /* ; : symbol */
#define TREAD 47      /* read : Keyword */
#define TWRITE 48     /* write : Keyword */
#define TBREAK 49     /* break : Keyword */
/**
 * @brief TokenKind enum
 * @enum TokenKind
 */
typedef enum {
  TK_NAME = 1,
  TK_PROGRAM,
  TK_VAR,
  TK_ARRAY,
  TK_OF,
  TK_BEGIN,
  TK_END,
  TK_IF,
  TK_THEN,
  TK_ELSE,
  TK_PROCEDURE,
  TK_RETURN,
  TK_CALL,
  TK_WHILE,
  TK_DO,
  TK_NOT,
  TK_OR,
  TK_DIV,
  TK_AND,
  TK_CHAR,
  TK_INTEGER,
  TK_BOOLEAN,
  TK_READLN,
  TK_WRITELN,
  TK_TRUE,
  TK_FALSE,
  TK_NUMBER,
  TK_STRING,
  TK_PLUS,
  TK_MINUS,
  TK_STAR,
  TK_EQUAL,
  TK_NOTEQ,
  TK_LE,
  TK_LEEQ,
  TK_GR,
  TK_GREQ,
  TK_LPAREN,
  TK_RPAREN,
  TK_LSQPAREN,
  TK_RSQPAREN,
  TK_ASSIGN,
  TK_DOT,
  TK_COMMA,
  TK_COLON,
  TK_SEMI,
  TK_READ,
  TK_WRITE,
  TK_BREAK
} TokenKind;

typedef struct Token Token;

/**
 * @brief Token type
 * @struct Token
 */
struct Token
{
  TokenKind kind;
  Token * next;
  int val;
  char * str;
};

#define NUMOFTOKEN 49

#define KEYWORDSIZE 28

#define S_ERROR -1

extern struct KEY
{
  char * keyword;
  int keytoken;
} key[KEYWORDSIZE];

int error(char * mes);

int init_scan(const char * path);
int scan(void);
int get_linenum(void);
void end_scan(void);

char * read_file(FILE * fp);

extern int num_attr;
extern char string_attr[];
extern FILE * fp;
extern int buf;
extern int cbuf;
extern uint line_num;

#endif
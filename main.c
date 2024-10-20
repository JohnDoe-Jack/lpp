#include <stdio.h>

#include "id-list.h"
#include "scan.h"

/* keyword list */
struct KEY key[KEYWORDSIZE] = {
  {"and", TAND},       {"array", TARRAY},         {"begin", TBEGIN},     {"boolean", TBOOLEAN},
  {"break", TBREAK},   {"call", TCALL},           {"char", TCHAR},       {"div", TDIV},
  {"do", TDO},         {"else", TELSE},           {"end", TEND},         {"false", TFALSE},
  {"if", TIF},         {"integer", TINTEGER},     {"not", TNOT},         {"of", TOF},
  {"or", TOR},         {"procedure", TPROCEDURE}, {"program", TPROGRAM}, {"read", TREAD},
  {"readln", TREADLN}, {"return", TRETURN},       {"then", TTHEN},       {"true", TTRUE},
  {"var", TVAR},       {"while", TWHILE},         {"write", TWRITE},     {"writeln", TWRITELN}};

/* Token counter */
int numtoken[NUMOFTOKEN + 1];

/* string of each token */
char * tokenstr[NUMOFTOKEN + 1] = {
  "",       "NAME",   "program",   "var",     "array",   "of",     "begin",   "end",  "if",
  "then",   "else",   "procedure", "return",  "call",    "while",  "do",      "not",  "or",
  "div",    "and",    "char",      "integer", "boolean", "readln", "writeln", "true", "false",
  "NUMBER", "STRING", "+",         "-",       "*",       "=",      "<>",      "<",    "<=",
  ">",      ">=",     "(",         ")",       "[",       "]",      ":=",      ".",    ",",
  ":",      ";",      "read",      "write",   "break"};

int main(int nc, char * np[])
{
  int token, i;

  if (nc < 2) {
    error("File name is not given.");
    return 0;
  }
  if (init_scan(np[1]) < 0) {
    error("Cannot open input file.");
    end_scan();
    return 0;
  }
  init_idtab();
  /* 作成する部分：トークンカウント用の配列？を初期化する */
  for (i = 0; i <= NUMOFTOKEN; i++) {
    numtoken[i] = 0;
  }

  while ((token = scan()) >= 0) {
    /* 作成する部分：トークンをカウントする */
    numtoken[token]++;
    if (token == TNAME) id_countup(string_attr);
  }
  end_scan();
  /* 作成する部分:カウントした結果を出力する */
  for (i = 1; i <= NUMOFTOKEN; i++) {
    if (numtoken[i] > 0) {
      printf("\"%s\"\t%d\n", tokenstr[i], numtoken[i]);
      if (i == 1) print_idtab();
    }
  }
  return 0;
}

/**
 * @brief エラーメッセージを表示してエラーを返す関数
 * 
 * @param mes 
 * @return int 
 */
int error(char * mes)
{
  fprintf(stderr, "\nERROR: %s\n", mes);
  return S_ERROR;
}

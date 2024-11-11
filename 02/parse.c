#include "lpp.h"

/**
 * @brief インデントの数をカウントする
 * 
 */
uint indent_cnt = 0;
/**
 * @brief ひとかたまりの文字列トークンを一時的に格納する
 * 
 */

void printIndent()
{
  for (uint i = 0; i < indent_cnt; i++) {
    printf("    ");
  }
}

void printVar() {}

void print(const uint token)
{
  printIndent();
  switch (token) {
    case TVAR:
      indent_cnt++;

      printf("var\n");
    case TSTRING:
      printf("%s", string_attr);
      break;
    case TPROGRAM:
      printf("program ");
      break;
    case TSEMI:
      printf(";\n");
      break;

    default:
      error("Invalid token", getLinenum());
      break;
  }
}
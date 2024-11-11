#include "print.h"

#include "scan.h"
#include "util.h"
/**
 * @brief インデントの数をカウントする
 * 
 */
uint indent_cnt = 0;
/**
 * @brief ひとかたまりの文字列トークンを一時的に格納する
 * 
 */

void print(const uint token)
{
  switch (token) {
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
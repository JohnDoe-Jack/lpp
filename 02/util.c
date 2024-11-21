#include <stdio.h>

#include "lpp.h"

/**
 * @brief エラーメッセージを表示する
 * 
 * @param fmt エラーメッセージ
 * @param line_num 行番号
 */
int error(char * fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  fflush(stderr);
  return ERROR;
}

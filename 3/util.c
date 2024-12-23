#include <stdio.h>

#include "lpp.h"

/**
 * @brief エラーメッセージを表示する
 * 
 * @param fmt エラーメッセージ
 */
TYPE_KIND error(char * fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  fflush(stderr);
  return ERROR;
}

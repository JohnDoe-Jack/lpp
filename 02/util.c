#include "util.h"

#include <stdio.h>
/**
 * @brief エラーメッセージを表示する
 * 
 * @param message エラーメッセージ
 * @param line_num 行番号
 */
void error(const char * message, const uint line_num)
{
  fprintf(stderr, "Error: %s.\nat %d line", message, line_num);
}

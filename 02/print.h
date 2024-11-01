#include <stdio.h>
#include <stdlib.h>

#include "util.h"

/**
 * @brief 初期化関数
 * プリティプリントを行う前に呼び出す
 * @param filename 
 * @return FILE* 
 */
FILE * initScan(char * filename)
{
  FILE * fin;
  if ((fin = fopen(filename, "r")) == NULL) {
    error("File not found.");
    exit(1);
  }
  return fin;
}
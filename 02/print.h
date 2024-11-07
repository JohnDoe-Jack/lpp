#ifndef PRINT_H
#define PRINT_H
#include <stdio.h>
#include <stdlib.h>

#include "util.h"

/**
 * @brief プリティプリントを行う関数
 * 前後の文脈から判断して適切なインデントを付けて標準出力する
 */
void print(const char *);

#endif
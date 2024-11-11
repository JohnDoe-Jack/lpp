#ifndef PRINT_H
#define PRINT_H

#include <sys/types.h>

#define SUCCESS 1
#define FAILURE 0

/**
 * @brief プリティプリントを行う関数
 * 前後の文脈から判断して適切なインデントを付けて標準出力する
 */
void print(const uint);
void printIndent();
void printVar();
extern uint indent_cnt;

#endif
#ifndef SCAN_H
#define SCAN_H
/**
 * @file
 * 字句解析を行う為の関数を纏めている
 */
/* scan.h  */
#include "lpp.h"

FILE * initScan(const char * path);

void endScan(void);

extern int num_attr;
extern char string_attr[];
extern FILE * fp;
extern int cbuf;
extern uint line_num, token_line_num;

#endif
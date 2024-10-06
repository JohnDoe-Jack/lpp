#include "scan.h"

int init_scan(char * filename)
{
  if ((fp = fopen(filename, "r")) == NULL) {
    return -1;
  }
  return 0;
}

int scan() {}

int get_linenum() {}

void end_scan() { fclose(fp); }
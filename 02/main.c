
#include "lpp.h"

bool file_exists(char *path) {
  struct stat st;
  return !stat(path, &st);
}


int main(int argc, char ** argv)
{
  int token;
  if (argc < 2) {
    error("File name is not given.", getLinenum());
    return -1;
  }

  FILE * fp;
  fp = initScan(argv[1]);

  while ((token = scan()) >= 0) {

  }

  return 0;
}

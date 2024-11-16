
#include "lpp.h"

bool file_exists(char * path)
{
  struct stat st;
  return !stat(path, &st);
}

int main(int argc, char ** argv)
{
  if (argc < 2) {
    error("File name is not given.");
    return -1;
  }
  if (!file_exists(argv[1])) {
    error("File not found: %s", argv[1]);
    return -1;
  }

  Token * tok = tokenizeFile(argv[1]);
  parse(tok);

  return 0;
}

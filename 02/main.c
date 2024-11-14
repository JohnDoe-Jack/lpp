
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
  Token * tok = tokenizeFile(argv[1]);
  int i = 0;
  while (tok->kind != TK_EOF) {
    printf(
      "%d: kind: %d, id: %d, len: %d, line_no: %d, str: %s, num: %d, at_bol: %d, has_space: %d\n",
      i++, tok->kind, tok->id, tok->len, tok->line_no, tok->str, tok->num, tok->at_bol,
      tok->has_space);
    tok = tok->next;
    if (i > 100) break;
  }
  return 0;
}

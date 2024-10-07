#include "scan.h"

/**
 * @brief 
 * 
 * @param pathに指定されたファイルを開く
 * @return int 
 */
int init_scan(const char * path)
{
  if ((fp = fopen(path, "r")) == NULL) {
    return -1;
  }
  cbuf = fgetc(fp);
  return 0;
}

char * read_file(FILE * fp)
{
  if (fseek(fp, 0, SEEK_END) == -1) {
    error(strerror(errno));
    return NULL;
  }
  size_t size = ftell(fp);
  if (fseek(fp, 0, SEEK_SET) == -1) {
    error(strerror(errno));
    return NULL;
  }

  char * buf = (char *)calloc(1, size + 2);
  fread(buf, size, 1, fp);

  if (size == 0 || buf[size - 1] != '\n') {
    buf[size++] = '\n';
  }
  buf[size] = '\0';
  return buf;
}

/**
 * @brief scan用の関数
 * @details 毎回二文字読み込む
 * @return int 
 */
int scan()
{
  for (;;) {
    if (cbuf == EOF) {
      return -1;
    }
    cbuf = fgetc(fp);

    switch (cbuf) {
      case ' ':
      case '\t':
        continue;
    }
    return 0;
  }
}

int get_linenum() { return 0; }

void end_scan() { fclose(fp); }

#include "lpp.h"

/**
 * @brief ファイルが存在するかをチェックする
 * 
 * @param path ファイルのパス
 * @return true ファイルが存在した場合
 * @return false ファイルが存在しなかった場合
 */
static bool file_exists(char * path)
{
  struct stat st;
  return !stat(path, &st);
}

static FILE * openFile(const char * path)
{
  FILE * out = fopen(path, "w");
  if (!out) error("Cannot open file: %s: %s", path, strerror(errno));
  return out;
}

static void getFileName(char * path, char * name)
{
  // パスのうち最後の'/'以降の部分を取り出す
  const char * lastSlash = strrchr(path, '/');
  if (!lastSlash) {
    // スラッシュが見つからなければ先頭から
    lastSlash = path;
  } else {
    // スラッシュをスキップ
    lastSlash++;
  }

  // ファイル名をコピー
  strcpy(name, lastSlash);

  // 最後に拡張子を取り除く（最低4文字ある場合のみ）
  int len = strlen(name);
  if (len > 4) {
    name[len - 4] = '\0';
  }
}

static void getOutputFileName(char * path, char * name)
{
  getFileName(path, name);
  strcat(name, ".csl");
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
  if (parse(tok) == ERROR) return ERROR;
  char * filename = (char *)malloc(sizeof(char) * MAXSTRSIZE);
  getOutputFileName(argv[1], filename);

  FILE * out = openFile(filename);
  if (codegen(tok, out) == ERROR) return ERROR;

  return 0;
}

#include "scan.h"

#include <ctype.h>
#include <string.h>

int num_attr;
char string_attr[MAXSTRSIZE];
FILE * fp;
int buf;
int cbuf;
uint line_num;

/**
 * @brief 
 * 
 * @param path
 * @return int 
 */
int init_scan(const char * path)
{
  if ((fp = fopen(path, "r")) == NULL) {
    return -1;
  }
  cbuf = fgetc(fp);
  line_num = 1;
  return 0;
}

void check_keyword()
{
  if (strncmp(string_attr, "program", strlen(string_attr))) {
  } else if (strncmp(string_attr, "array", strlen(string_attr))) {
  }
}

/**
 * @brief 改行をチェックして、行番号をインクリメントする関数
 * @details \r,\n,\r\n,\n\rの四種類の改行を見分ける
 */
void check_newline()
{
  if (cbuf == '\r') {
    cbuf = fgetc(fp);
    if (cbuf == '\n') {
      line_num++;
    } else {
      line_num++;
    }
  } else if (cbuf == '\n') {
    cbuf = fgetc(fp);
    if (cbuf == '\r') {
      line_num++;
    } else {
      line_num++;
    }
  }
}

/**
 * @brief scan()は、ファイルから1文字ずつ読み込んで、トークンを切り出す関数
 * @return int 
 */
int scan()
{
  for (;;) {
    if (cbuf == EOF) {
      return -1;
    }
    switch (cbuf) {
      // 空白とタブは読み飛ばす
      case ' ':
      case '\t':
        cbuf = fgetc(fp);
        break;
      // 改行文字 (\n または \r)
      case '\n':
      case '\r':
        check_newline();  // 行番号の更新
        cbuf = fgetc(fp);
        break;
      // {}による注釈を読み飛ばす
      case '{':
        while ((cbuf = fgetc(fp)) != '}')
          if (cbuf == EOF) return -1;
        break;
      // /* */による注釈を読み飛ばす
      case '/':
        if ((cbuf = fgetc(fp)) == '*') {
          // */が出るまで読み飛ばす
          while (1) {
            while ((cbuf = fgetc(fp)) != '*')
              if (cbuf == EOF) return -1;
            if ((cbuf = fgetc(fp)) == '/') break;
          }
        }
        break;
    }

    if (isalpha(cbuf)) {
      // 名前の読み込み
      int i = 0;
      do {
        string_attr[i++] = cbuf;
        cbuf = fgetc(fp);
      } while (isalnum(cbuf));
      string_attr[i] = '\0';

      check_keyword();

      return TNAME;
    } else if (isdigit(cbuf)) {
      // 数字の読み込み
      num_attr = 0;
      do {
        num_attr = num_attr * 10 + (cbuf - '0');
        cbuf = fgetc(fp);
      } while (isdigit(cbuf));
      return TNUMBER;
    } else {
      // 1文字のトークン
      switch (cbuf) {
        case '+':
          cbuf = fgetc(fp);
          return TPLUS;
        case '-':
          cbuf = fgetc(fp);
          return TMINUS;
        case '*':
          cbuf = fgetc(fp);
          return TSTAR;
        case '=':
          cbuf = fgetc(fp);
          return TEQUAL;
        case '<':
          cbuf = fgetc(fp);
          if (cbuf == '=') {
            cbuf = fgetc(fp);
            return TLEEQ;
          } else if (cbuf == '>') {
            cbuf = fgetc(fp);
            return TNOTEQ;
          } else {
            return TLE;
          }
        case '>':
          cbuf = fgetc(fp);
          if (cbuf == '=') {
            cbuf = fgetc(fp);
            return TGREQ;
          } else {
            return TGR;
          }
        case '(':
          cbuf = fgetc(fp);
          return TLPAREN;
        case ')':
          cbuf = fgetc(fp);
          return TRPAREN;
        case '[':
          cbuf = fgetc(fp);
          return TLSQPAREN;
        case ']':
          cbuf = fgetc(fp);
          return TRSQPAREN;
        case ':':
          cbuf = fgetc(fp);
          if (cbuf == '=') {
            cbuf = fgetc(fp);
            return TASSIGN;
          } else {
            return TCOLON;
          }
        case '.':
          cbuf = fgetc(fp);
          return TDOT;
        case ',':
          cbuf = fgetc(fp);
          return TCOMMA;
        case ';':
          cbuf = fgetc(fp);
          return TSEMI;
        case EOF:
          return -1;
        default:
          printf("Illegal character: %d\n", cbuf);
          cbuf = fgetc(fp);
          break;
          return -1;
      }
    }
  }
}

int get_linenum() { return line_num; }

void end_scan() { fclose(fp); }
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

/**
 * @brief keyword listに含まれていかをチェックする
 *
 */
int check_keyword()
{
  if (strcmp(string_attr, "program") == 0) {
    return TPROGRAM;
  } else if (strcmp(string_attr, "var") == 0) {
    return TVAR;
  } else if (strcmp(string_attr, "array") == 0) {
    return TARRAY;
  } else if (strcmp(string_attr, "of") == 0) {
    return TOF;
  } else if (strcmp(string_attr, "begin") == 0) {
    return TBEGIN;
  } else if (strcmp(string_attr, "end") == 0) {
    return TEND;
  } else if (strcmp(string_attr, "if") == 0) {
    return TIF;
  } else if (strcmp(string_attr, "then") == 0) {
    return TTHEN;
  } else if (strcmp(string_attr, "else") == 0) {
    return TELSE;
  } else if (strcmp(string_attr, "procedure") == 0) {
    return TPROCEDURE;
  } else if (strcmp(string_attr, "return") == 0) {
    return TRETURN;
  } else if (strcmp(string_attr, "call") == 0) {
    return TCALL;
  } else if (strcmp(string_attr, "while") == 0) {
    return TWHILE;
  } else if (strcmp(string_attr, "do") == 0) {
    return TDO;
  } else if (strcmp(string_attr, "not") == 0) {
    return TNOT;
  } else if (strcmp(string_attr, "or") == 0) {
    return TOR;
  } else if (strcmp(string_attr, "div") == 0) {
    return TDIV;
  } else if (strcmp(string_attr, "and") == 0) {
    return TAND;
  } else if (strcmp(string_attr, "char") == 0) {
    return TCHAR;
  } else if (strcmp(string_attr, "integer") == 0) {
    return TINTEGER;
  } else if (strcmp(string_attr, "boolean") == 0) {
    return TBOOLEAN;
  } else if (strcmp(string_attr, "readln") == 0) {
    return TREADLN;
  } else if (strcmp(string_attr, "writeln") == 0) {
    return TWRITELN;
  } else if (strcmp(string_attr, "true") == 0) {
    return TTRUE;
  } else if (strcmp(string_attr, "false") == 0) {
    return TFALSE;
  } else if (strcmp(string_attr, "read") == 0) {
    return TREAD;
  } else if (strcmp(string_attr, "write") == 0) {
    return TWRITE;
  } else if (strcmp(string_attr, "break") == 0) {
    return TBREAK;
  } else {
    return TNAME;
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
        continue;
      // 改行文字 (\n または \r)
      case '\n':
      case '\r':
        check_newline();  // 行番号の更新
        continue;
      // {}による注釈を読み飛ばす
      case '{':
        while ((cbuf = fgetc(fp)) != '}') {
          if (cbuf == EOF) return -1;
        }
        cbuf = fgetc(fp);
        continue;
      // /* */による注釈を読み飛ばす
      case '/':
        cbuf = fgetc(fp);
        if (cbuf == '*') {
          // */が出るまで読み飛ばす
          while (1) {
            while ((cbuf = fgetc(fp)) != '*') {
              if (cbuf == EOF) return -1;
            }
            if ((cbuf = fgetc(fp)) == '/') {
              break;
            }
          }
          cbuf = fgetc(fp);
          continue;
        } else {
          printf("Illegal character: %c\n", cbuf);
          return -1;
        }
    }

    if (isalpha(cbuf)) {
      // 名前の読み込み
      int i = 0;
      do {
        string_attr[i++] = cbuf;
        if (i < MAXSTRSIZE - 1)
          string_attr[i] = '\0';
        else
          return -1;
        cbuf = fgetc(fp);
      } while (isalnum(cbuf));
      return check_keyword();
    } else if (isdigit(cbuf)) {
      // 数字の読み込み
      num_attr = 0;
      do {
        num_attr = num_attr * 10 + (cbuf - '0');
        if (num_attr > MAXNUM) return -1;
        cbuf = fgetc(fp);
      } while (isdigit(cbuf));
      return TNUMBER;
    } else if (cbuf == '\'') {
      // 'で囲まれた文字列の読み込み
      int i = 0;
      cbuf = fgetc(fp);  // 最初の'を読み飛ばす
      for (;;) {
        if (cbuf == EOF) return -1;  // 'で閉じる前にEOFになった場合
        if (cbuf == '\'') {
          cbuf = fgetc(fp);
          if (cbuf != '\'') break;
        }
        string_attr[i++] = cbuf;
        cbuf = fgetc(fp);
      }

      if (cbuf == EOF) return -1;  // 'で閉じる前にEOFになった場合
      string_attr[i] = '\0';
      return TSTRING;
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
          printf("Illegal character: %c\n", cbuf);
          printf("line: %d\n", line_num);
          cbuf = fgetc(fp);
          return -1;
      }
    }
  }
}

int get_linenum() { return line_num; }

void end_scan() { fclose(fp); }
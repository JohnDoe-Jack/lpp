#include "scan.h"

#include <ctype.h>
#include <string.h>

/**
 * @file
 * 字句解析を行う為の関数を纏めている
 */

/**
 * @brief ひとかたまりの数字トークンを一時的に格納する
 * 
 */
int num_attr;
/**
 * @brief ひとかたまりの文字列トークンを一時的に格納する
 * 
 */
char string_attr[MAXSTRSIZE];
/**
 * @brief 解析するファイルを指すポインタ
 * 
 */
FILE * fp;

/**
 * @brief ファイルから先読みした1文字を一時的に格納する
 * 
 */
int cbuf;

/**
 * @brief 先読みした時点での行番号を格納する変数
 * 
 */
uint line_num;

/**
 * @brief トークンの行番号を格納する変数
 * 
 */
uint token_line_num;

/**
 * @brief 字句解析前に呼び出される関数
 * 正常にファイルを開けた場合は0を返す、失敗した場合は-1を返す
 * @param path
 * @return int 
 */
int init_scan(const char * path)
{
  if ((fp = fopen(path, "r")) == NULL) {
    return S_ERROR;
  }
  cbuf = fgetc(fp);
  line_num = 1;
  return 0;
}

/**
 * @brief keyword listに含まれていかをチェックする
 * 予約語が含まれていた場合、そのトークンの種類を返す
 * @return int
 */
int check_keyword()
{
  for (int i = 0; i < KEYWORDSIZE; i++) {
    if (strcmp(string_attr, key[i].keyword) == 0) {
      return key[i].keytoken;
    }
  }
  return TNAME;
}

/**
 * @brief 改行をチェックして、行番号をインクリメントする関数
 * @details \r,\\n,\r\\n,\\n\rの四種類の改行を見分ける
 * @return void
 */
void check_newline()
{
  if (
    (cbuf == '\r' && (cbuf = fgetc(fp)) == '\n') || (cbuf == '\n' && (cbuf = fgetc(fp)) == '\r')) {
    cbuf = fgetc(fp);
  }
  line_num++;
}

/**
 * @brief scan()は、ファイルから1文字ずつ読み込んで、トークンを切り出す関数
 * 大域変数cbufには1文字先読みした文字が格納されており、トークンの種類が決定するまでcbufを更新していく
 * @return int 
 * @details トークンの種類を返す
 */
int scan()
{
  // トークンの行番号を記録
  token_line_num = line_num;
  for (;;) {
    if (cbuf == EOF) {
      return S_ERROR;
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
          if (cbuf == EOF) return S_ERROR;
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
              if (cbuf == EOF) return S_ERROR;
            }
            if ((cbuf = fgetc(fp)) == '/') {
              break;
            }
          }
          cbuf = fgetc(fp);
          continue;
        } else {
          printf("Illegal character: %c\n", cbuf);
          return S_ERROR;
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
          return S_ERROR;
        cbuf = fgetc(fp);
      } while (isalnum(cbuf));
      return check_keyword();
    } else if (isdigit(cbuf)) {
      // 数字の読み込み
      num_attr = 0;
      do {
        num_attr = num_attr * 10 + (cbuf - '0');
        if (num_attr > MAXNUM) return S_ERROR;
        cbuf = fgetc(fp);
      } while (isdigit(cbuf));
      return TNUMBER;
    } else if (cbuf == '\'') {
      // 'で囲まれた文字列の読み込み
      int i = 0;
      cbuf = fgetc(fp);  // 最初の'を読み飛ばす
      for (;;) {
        if (i >= MAXSTRSIZE - 1) return S_ERROR;
        if (cbuf == EOF) return S_ERROR;  // 'で閉じる前にEOFになった場合
        if (cbuf == '\'') {
          cbuf = fgetc(fp);
          if (cbuf != '\'') break;
        }
        string_attr[i++] = cbuf;
        cbuf = fgetc(fp);
      }

      if (cbuf == EOF) return S_ERROR;  // 'で閉じる前にEOFになった場合
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
        case '.':
          cbuf = fgetc(fp);
          return TDOT;
        case ',':
          cbuf = fgetc(fp);
          return TCOMMA;
        case ';':
          cbuf = fgetc(fp);
          return TSEMI;
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

        case ':':
          cbuf = fgetc(fp);
          if (cbuf == '=') {
            cbuf = fgetc(fp);
            return TASSIGN;
          } else {
            return TCOLON;
          }
        default:
          printf("Illegal character: %c\n", cbuf);
          printf("line: %d\n", line_num);
          cbuf = fgetc(fp);
          return S_ERROR;
      }
    }
  }
}

/**
 * @brief 行番号を返す関数
 * token_line_numは、1個のトークンが確定した時点での行番号を示す
 * @return int 
 */
int get_linenum() { return token_line_num; }

/**
 * @brief 字句解析が終わったあとに呼び出される関数
 * fpは解析するファイルを指すポインタで、解析が終わったら閉じる
 */
void end_scan() { fclose(fp); }
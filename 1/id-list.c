#include "id-list.h"

#include "scan.h"

/**
 * @file
 * 構文中に出現した文字列を記録する
 */

/**
 * @brief 出現した名前を表す構造体
 * @struct ID
 */
struct ID
{
  //! 名前
  char * name;
  //! 出現回数
  int count;
  //! 次のID構造体へのポインタ
  struct ID * nextp;
} * idroot;

/**
 * @brief 名前のリストを初期化する
 * 
 */
void init_idtab()
{ /* Initialise the table */
  idroot = NULL;
}

/**
 * @brief 名前を検索する
 * もし以前に登録された名前であれば、そのID構造体へのポインタを返す
 * @param np 
 * @return struct ID* 
 */
struct ID * search_idtab(char * np)
{ /* search the name pointed by np */
  struct ID * p;

  for (p = idroot; p != NULL; p = p->nextp) {
    if (!strcmp(np, p->name)) return (p);
  }
  return (NULL);
}

/**
 * @brief 名前を登録してカウントアップする
 * 今までに登録された名前であれば、カウントをインクリメントする。
 * そうでなければ、新たにID構造体を作成し、名前を登録する
 * @param np 
 */
void id_countup(char * np)
{ /* Register and count up the name pointed by np */
  struct ID * p;
  char * cp;

  if ((p = search_idtab(np)) != NULL)
    p->count++;
  else {
    if ((p = (struct ID *)malloc(sizeof(struct ID))) == NULL) {
      error("Cannot malloc for p in id_countup");
      return;
    }
    if ((cp = (char *)malloc(strlen(np) + 1)) == NULL) {
      error("Cannot malloc for cp in id_countup");
      return;
    }
    strcpy(cp, np);
    p->name = cp;
    p->count = 1;
    p->nextp = idroot;
    idroot = p;
  }
}

/**
 * @brief 登録されたデータを出力する
 * 
 */
void print_idtab()
{ /* Output the registered data */
  struct ID * p;

  for (p = idroot; p != NULL; p = p->nextp) {
    if (p->count != 0) printf("\t\"Identifier\" \"%s\"\t%d\n", p->name, p->count);
  }
}

/**
 * @brief 登録されたデータを解放する
 * 
 */
void release_idtab()
{ /* Release tha data structure */
  struct ID *p, *q = NULL;

  for (p = idroot; p != NULL; p = q) {
    free(p->name);
    q = p->nextp;
    free(p);
  }
  init_idtab();
}

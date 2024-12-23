/**
 * @file
 * 構文中に出現した文字列を記録する
 */

#ifndef ID_LIST_H
#define ID_LIST_H

void init_idtab();
struct ID * search_idtab(char * np);
void id_countup(char * np);
void print_idtab();
void release_idtab();

#endif
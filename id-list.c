#include "scan.h"

struct ID {
  char *name;
  int count;
  struct ID *nextp;
} *idroot;

void init_idtab() { /* Initialise the table */
  idroot = NULL;
}

struct ID *search_idtab(char *np) { /* search the name pointed by np */
  struct ID *p;

  for (p = idroot; p != NULL; p = p->nextp) {
    if (!strcmp(np, p->name))
      return (p);
  }
  return (NULL);
}

void id_countup(char *np) { /* Register and count up the name pointed by np */
  struct ID *p;
  char *cp;

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

void print_idtab() { /* Output the registered data */
  struct ID *p;

  for (p = idroot; p != NULL; p = p->nextp) {
    if (p->count != 0)
      printf("\t\"Identifier\" \"%s\"\t%d\n", p->name, p->count);
  }
}

void release_idtab() { /* Release tha data structure */
  struct ID *p, *q = NULL;

  for (p = idroot; p != NULL; p = q) {
    free(p->name);
    q = p->nextp;
    free(p);
  }
  init_idtab();
}

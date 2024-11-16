#include <stdio.h>

#include "lpp.h"

#define ERROR 1
#define NORMAL 0

static int indent_level = 0;
static bool at_bol = false;
static Token * cur;
static const char * token_str[NUMOFTOKEN + 1] = {
  "",       "NAME",   "program",   "var",     "array",   "of",     "begin",   "end",  "if",
  "then",   "else",   "procedure", "return",  "call",    "while",  "do",      "not",  "or",
  "div",    "and",    "char",      "integer", "boolean", "readln", "writeln", "true", "false",
  "NUMBER", "STRING", "+",         "-",       "*",       "=",      "<>",      "<",    "<=",
  ">",      ">=",     "(",         ")",       "[",       "]",      ":=",      ".",    ",",
  ":",      ";",      "read",      "write",   "break"};
static void printIndent()
{
  for (int i = 0; i < indent_level; i++) {
    printf("    ");
  }
}

static void printToken(const Token * tok)
{
  if (tok->has_space) printf(" ");
  if (tok->at_bol || at_bol) {
    printf("\n");
    printIndent();
  }

  if (tok->kind == TK_KEYWORD || tok->kind == TK_PUNCT) {
    printf("%s", token_str[cur->id]);
  } else if (cur->kind == TK_NUM) {
    printf("%d", cur->num);
  } else if (cur->kind == TK_STR) {
    printf("%s", cur->str);
  } else {
    printf("%s", cur->str);
  }
  fflush(stdout);
}

static int parseVarDeclaration()
{
  if (cur->id != TVAR) return error("Expected 'var'", cur->line_no);
  indent_level++;
  printIndent();
  printToken(cur);
  cur = cur->next;

  return NORMAL;
}

static int parseBlock()
{
  if (cur->id == TVAR) parseVarDeclaration();
  return NORMAL;
}

static int parseProgram()
{
  if (cur->id != TPROGRAM)
    return error("Expected 'program' at the beginning of the program", cur->line_no);

  printToken(cur);
  cur = cur->next;
  if (cur->id != TNAME) return error("Expected program name", cur->line_no);

  printToken(cur);
  cur = cur->next;
  if (cur->id != TSEMI) return error("Expected ';'", cur->line_no);

  printToken(cur);
  cur = cur->next;
  if (parseBlock() == ERROR) return ERROR;
  if (cur->id != TDOT) return error("Expected '.'", cur->line_no);
  printToken(cur);
  cur = cur->next;

  return NORMAL;
}

void parse(Token * tok)
{
  cur = tok;
  //   for (; cur->kind != TK_EOF; tok = cur->next) {
  //     printToken(tok);
  //   }
  parseProgram();
}
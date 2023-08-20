#include "csapp.h"
#include <stddef.h>
#ifndef _NODE
#define _NODE
typedef struct _node {
  char key[MAXLINE];
  char *value;
  struct _node *next;
  size_t size;
} Node, *linkList;
Node *creatNode(char *key, char *value);
void listInit(linkList *list);
void put(linkList list, Node *node);
Node top(linkList list);
Node *getNode(linkList list, char *key);
void printNode(Node *node);
unsigned long getTotalSize(linkList list, Node **node);
Node *getTail(linkList list);
void moveTop(linkList list, Node *node);
void showList(linkList list);
#endif
#include "linklist.h"
#include <setjmp.h>
#include <stdio.h>

void listInit(linkList *list) { *list = creatNode(NULL, NULL); }
Node *creatNode(char *key, char *value) {
  Node *nodePtr;
  nodePtr = (Node *)malloc(sizeof(Node));
  if (nodePtr == NULL) {
    app_error("creatNode error");
  }
  nodePtr->next = NULL;
  if (key && value) {
    strcpy(nodePtr->key, key);
    nodePtr->value = value;
  }
  return nodePtr;
}
void put(linkList list, Node *node) {
  if (list && node) {
    node->next = list->next;
    list->next = node;
  } else {
    app_error("put error: NUll Point");
  }
}
Node top(linkList list) {
  Node *p = list->next;
  list->next = list->next->next;
  Node ret = *p;
  free(p);
  return ret;
}
Node *getNode(linkList list, char *key) {
  if (list == NULL) {
    app_error("getNode error:");
  }
  Node *p = list->next;
  while (p) {
    if (!strcmp(p->key, key)) {
      break;
    }
    p = p->next;
  }
  return p;
}
void printNode(Node *node) {
  if (node)
    printf("(%s,%s)", node->key, node->value);
}
unsigned long getTotalSize(linkList list, Node **node) {
  Node *p = list->next;
  unsigned long retVal = 0;
  while (p) {
    retVal += p->size;
  }
  if (*node)
    *node = p;
  return retVal;
}
/*
  move a node to top of link list
*/
void moveTop(linkList list, Node *node) {
  // except NULL and only head node and only have one node
  if (list == NULL || list->next == NULL || list->next->next == NULL)
    return;
  // get previous node of the augment node
  Node *pre = list;
  Node *curr = node;
  while (pre->next && pre->next != node) {
    pre = pre->next;
  }
  // already top node
  if (pre == list)
    return;
  // mid node
  if (curr->next) {
    pre->next = curr->next;
    put(list, curr);
  }
  // end node
  else {
    pre->next = NULL;
    put(list, curr);
  }
}
void showList(linkList list) {
  Node *node = list->next;
  while (node) {
    printNode(node);
    node = node->next;
  }
  printf("\r\n");
}
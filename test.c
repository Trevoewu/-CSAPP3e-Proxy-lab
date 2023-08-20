#include "csapp.c"
#include "csapp.h"
#include "linklist.c"
#include "linklist.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
int main() {
  char *path = "/index";
  char *host = "localhost";
  char key[MAXLINE] = "";
  strcat(key, host);
  strcat(key, path);
  linkList list;
  listInit(&list);
  put(list, creatNode("1", "A"));
  put(list, creatNode("2", "B"));
  put(list, creatNode("3", "C"));
  put(list, creatNode("4", "D"));
  put(list, creatNode("5", "E"));
  showList(list);
  moveTop(list, getNode(list, "1"));
  showList(list);
}
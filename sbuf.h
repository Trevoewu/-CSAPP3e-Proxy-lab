#ifndef __SBUF_H__
#define __SBUF_H__

#include "csapp.h"
#include "rk_sem.h"
/*structure share buf*/
typedef struct {
  int *buf;  // Buffer array to store items(socket descriptors)
  int n;     // Maximum number of slots
  int front; // buf[(++front)%n] is first item and the earnest item produced
  int rear;  // buf[rear%n] is last item And the latest item produced
  struct rk_sema mutex; // protects accesses to buf
  struct rk_sema slots; // counts available slots
  struct rk_sema items; // counts available items
} sbuf_t;

/*share buf(sbuf) initiate*/
void init_sbuf(sbuf_t *sp, int n);
/*insert item to share buf*/
void insert_sbuf(sbuf_t *sp, int item);
/*remove item from buf , and return the item removed */
int remove_sbuf(sbuf_t *sp);
/*judge buf is empty*/
int isEmpty(sbuf_t *sp);
/*judge buf is full*/
int isFully(sbuf_t *sp);
/*get the amount of item*/
int getAmount(sbuf_t *sp);
#endif /* __SBUF_H__ */
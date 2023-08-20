/* $begin sbufc */
#include "sbuf.h"
#include "csapp.h"
int isEmpty(sbuf_t *sp) { return sp->front == sp->rear ? 1 : 0; }
int isFull(sbuf_t *sp) { return (sp->rear - sp->front) == sp->n ? 1 : 0; }
void init_sbuf(sbuf_t *sp, int n) {
  sp->buf = Calloc(n, sizeof(int));
  sp->front = 0;
  sp->rear = 0;

  rk_sema_init(&sp->mutex, 1);
  rk_sema_init(&sp->slots, n);
  rk_sema_init(&sp->items, 0);
}
void insert_sbuf(sbuf_t *sp, int item) {
  // wait a slot
  rk_sema_wait(&sp->slots);
  // lock the buf
  rk_sema_wait(&sp->mutex);
  // add a item into buf
  sp->buf[(++sp->rear) % sp->n] = item;
  // unlock the buf
  rk_sema_post(&sp->mutex);
  // announce customer available item
  rk_sema_post(&sp->items);
}
int remove_sbuf(sbuf_t *sp) {
  // wait a item
  rk_sema_wait(&sp->items);
  // lock buf
  rk_sema_wait(&sp->mutex);
  // pop a item
  int item = sp->buf[(++sp->front) % sp->n];
  // unlock buffer
  rk_sema_post(&sp->mutex);
  // announce a available slot
  rk_sema_post(&sp->slots);
  return item;
}
int getAmount(sbuf_t *sp) { return sp->rear - sp->front; }
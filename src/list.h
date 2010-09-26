#ifndef LIST_H
#define LIST_H

#include <errno.h>
#include <pthread.h>
#include <malloc.h>

typedef struct litem_ litem;
struct litem_ {
	litem *next;
	void  *item;
};

typedef struct list_ list;
struct list_ {
	litem            *head;
	litem     	 *tail;
	pthread_mutex_t   lock;
};

typedef  void * (*iter_callback)(void *, void *, void *);

#define ITER_CONTINUE 1
#define ITER_BREAK    0
#define ITER_BROKEN   1

void  list_init   (list *clist);
void  list_push   (list *clist, void *item);
void* list_pop    (list *clist);
int   list_iter   (list *clist, iter_callback func, void *arg1, void *arg2);
int   list_unlink (list *clist, void *item);

#endif // LIST_H

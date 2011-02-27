#ifndef LIST_H
#define LIST_H

#include <errno.h>
#include <pthread.h>
#include <malloc.h>

typedef struct litem_ litem;
struct litem_ {
	litem *lnext;
	void  *item;
};

typedef struct list_ list;
struct list_ {
	litem            *head;
	litem     	 *tail;
	pthread_mutex_t   lock;
};

typedef int  (*iter_callback)(void *, void *, void *);

#define ITER_OK       0
#define ITER_BREAK    1
#define ITER_CONTINUE 2

void  list_init   (list *clist);
void  list_destroy(list *clist);
void  list_push   (list *clist, void *item);
void* list_pop    (list *clist);
int   list_iter   (list *clist, iter_callback func, void *arg1, void *arg2);
int   list_unlink (list *clist, void *item);

#endif // LIST_H

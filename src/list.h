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

void  list_init(list *clist);
void  list_push(list *clist, void *item);
void* list_pop(list *clist);
void  list_iter(list *clist, void *(*func)(void *, void *, void *), void *arg1, void *arg2);
void  list_unlink(list *clist, void *item);


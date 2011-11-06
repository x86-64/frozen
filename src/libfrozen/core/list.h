#ifndef LIST_H
#define LIST_H

typedef struct list {
	pthread_rwlock_t       lock;
	void                 **items;
} list;

#define LIST_FREE_ITEM    (void *)-1
#define LIST_END          (void *)NULL
#define LIST_INITIALIZER  { .items = NULL, .lock = PTHREAD_RWLOCK_INITIALIZER }

#define ITER_OK       0
#define ITER_BREAK    1
#define ITER_CONTINUE 2

void       list_init          (list *clist);
void       list_destroy       (list *clist);
void       list_add           (list *clist, void *item);
void       list_delete        (list *clist, void *item);
intmax_t   list_is_empty      (list *clist);
uintmax_t  list_count         (list *clist);
void       list_flatten       (list *clist, void **memory, size_t items);
void       list_rdlock        (list *clist);
void       list_wrlock        (list *clist);
void       list_unlock        (list *clist);
void *     list_iter_next     (list *clist, void **iter);
void       list_push          (list *clist, void *item);
void *     list_pop           (list *clist);

#endif // LIST_H

#include <list.h>

void list_init(list *clist){
	clist->head = NULL;
	clist->tail = NULL;
	pthread_mutex_init(&clist->lock, NULL);
}

void list_push(list *clist, void *item){
	
	litem *new_litem = (litem *)malloc(sizeof(litem));
	if(new_litem != NULL){
		new_litem->item = item;
		new_litem->next = NULL;

		pthread_mutex_lock(&clist->lock);
			if(clist->tail == NULL){
				clist->head = new_litem;
			}else{
				clist->tail->next = new_litem;
			}
			clist->tail = new_litem;	
		pthread_mutex_unlock(&clist->lock);
	}
}

void* list_pop(list *clist){
	litem *poped_item;

	pthread_mutex_lock(&clist->lock);
		poped_item = clist->head;
		if(poped_item != NULL){
			clist->head = poped_item->next;
			if(clist->head == NULL)
				clist->tail = NULL;
		}
	pthread_mutex_unlock(&clist->lock);
	if(poped_item == NULL)
		return NULL;
	void *item = poped_item->item;
	free(poped_item);
	return item;
}

void list_iter(list *clist, void *(*func)(void *, void *, void *), void *arg1, void *arg2){
	pthread_mutex_lock(&clist->lock);
	
	litem *curr;
	curr = clist->head;
	while(curr != NULL){
		if(func(curr->item, arg1, arg2) == NULL)
			break;
		curr = curr->next;
	}

	pthread_mutex_unlock(&clist->lock);
}

void list_unlink(list *clist, void *item){
	pthread_mutex_lock(&clist->lock);
	
	litem *curr, *last;
	last = NULL;
	curr = clist->head;
	while(curr != NULL){
		if(curr->item == item){
			if(last != NULL)
				last->next = curr->next;
			if(clist->tail == curr)
				clist->tail = last;
			if(clist->head == curr)
				clist->head = curr->next;
			free(curr);
			break;
		}
		last = curr;
		curr = curr->next;
	}
	
	pthread_mutex_unlock(&clist->lock);
}


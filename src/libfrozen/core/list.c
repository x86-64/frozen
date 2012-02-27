#include <libfrozen.h>
#include <list.h>

list *     list_alloc(void){ // {{{
	list                  *new_list;
	
	if( (new_list = malloc(sizeof(list))) == NULL)
		return NULL;
	
	list_init(new_list);
	return new_list;
} // }}}
void       list_free(list *list){ // {{{
	list_destroy(list);
	free(list);
} // }}}
void       list_init(list *clist){ // {{{
	clist->items = NULL;
	pthread_rwlock_init(&clist->lock, NULL);
} // }}}
void       list_destroy(list *clist){ // {{{
	pthread_rwlock_wrlock(&clist->lock);
		if(clist->items != NULL){
			free(clist->items);
			clist->items = NULL;
		}
	pthread_rwlock_unlock(&clist->lock);
} // }}}
void       list_add    (list *clist, void *item){ // {{{
	void                 **list;
	void                  *curr;
	size_t                 lsz      = 0;
	
	pthread_rwlock_wrlock(&clist->lock);
	
	if( (list = clist->items) != NULL){
		while( (curr = list[lsz]) != LIST_END){
			if(curr == LIST_FREE_ITEM)
				goto found;
			
			lsz++;
		}
	}
	list = clist->items = realloc(list, sizeof(void *) * (lsz + 2));
	list[lsz + 1] = LIST_END;
found:
	list[lsz + 0] = item;
	
	pthread_rwlock_unlock(&clist->lock);
} // }}}
void       list_push   (list *clist, void *item){ // {{{
	void                 **list;
	void                  *curr;
	size_t                 lsz      = 0;
	
	pthread_rwlock_wrlock(&clist->lock);
	
	if( (list = clist->items) != NULL){
		while( (curr = list[lsz]) != LIST_END){
			if(curr == LIST_FREE_ITEM)
				goto found;
			
			lsz++;
		}
	}
	list = clist->items = realloc(list, sizeof(void *) * (lsz + 2));
	memmove(list + 1, list, sizeof(void *) * (lsz + 1));
	list[lsz + 1] = LIST_END;
	lsz = 0;
found:
	list[lsz + 0] = item;
	
	pthread_rwlock_unlock(&clist->lock);
} // }}}
size_t     list_delete (list *clist, void *item){ // {{{
	void                 **list;
	void                  *curr;
	size_t                 lsz = 0;
	size_t                 ret = 0;
	
	pthread_rwlock_wrlock(&clist->lock);
	
	if( (list = clist->items) == NULL)
		goto exit;
	
	while( (curr = list[lsz]) != LIST_END){
		if(curr == item){
			list[lsz] = LIST_FREE_ITEM;
			ret = 1;
			goto exit;
		}
		
		lsz++;
	}
exit:
	pthread_rwlock_unlock(&clist->lock);
	return ret;
} // }}}
intmax_t   list_is_empty(list *clist){ // {{{
	void                 **list;
	void                  *curr;
	size_t                 lsz = 0, ret;
	
	pthread_rwlock_rdlock(&clist->lock);
	
	if( (list = clist->items) == NULL)
		goto fail;
	
	while( (curr = list[lsz]) != LIST_END){
		if(curr != LIST_FREE_ITEM){
			ret = 0; goto exit;
		}
		
		lsz++;
	}
fail:
	ret = 1;
exit:
	pthread_rwlock_unlock(&clist->lock);
	return ret;
} // }}}
uintmax_t  list_count(list *clist){ // {{{
	void                 **list;
	void                  *curr;
	size_t                 lsz = 0, size = 0;
	
	if( (list = clist->items) == NULL)
		goto exit;
	
	while( (curr = list[lsz++]) != LIST_END){
		if(curr == LIST_FREE_ITEM)
			continue;
		
		size++;
	}
	if(size == 0)
		goto exit;
exit:
	return size;
} // }}}
void       list_flatten(list *clist, void **memory, size_t items){ // {{{
	void                 **list;
	void                  *curr;
	size_t                 i = 0, lsz = 0;
	
	list = clist->items;
	while( (curr = list[lsz++]) != LIST_END && i < items){
		if(curr == LIST_FREE_ITEM)
			continue;
		
		memory[i++] = curr;
	}
} // }}}
void       list_rdlock(list *clist){ // {{{
	pthread_rwlock_rdlock(&clist->lock);
} // }}}
void       list_wrlock(list *clist){ // {{{
	pthread_rwlock_wrlock(&clist->lock);
} // }}}
void       list_unlock(list *clist){ // {{{
	pthread_rwlock_unlock(&clist->lock);
} // }}}
void *     list_iter_next(list *clist, void **iter){ // {{{
	void                 **list;
	void                  *curr;
	size_t                 lsz = 0;
	
	list = (*iter == NULL) ? clist->items : *iter;
	if(list != NULL){ 
		while( (curr = list[lsz++]) != LIST_END){
			if(curr == LIST_FREE_ITEM)
				continue;
			
			*iter = &list[lsz];
			return curr;
		}
	}
	return NULL;
} // }}}
void *     list_pop(list *clist){ // {{{
	void                 **list;
	void                  *curr = NULL;
	size_t                 lsz = 0;
	
	pthread_rwlock_wrlock(&clist->lock);
	
	if( (list = clist->items) != NULL){
		while( (curr = list[lsz]) != LIST_END){
			if(curr != LIST_FREE_ITEM){
				list[lsz] = LIST_FREE_ITEM;
				goto exit;
			}
			
			lsz++;
		}
	}
exit:
	pthread_rwlock_unlock(&clist->lock);
	return curr;
} // }}}


/*
void list_push(list *clist, void *item){
	litem *new_litem = (litem *)malloc(sizeof(litem));
	if(new_litem != NULL){
		new_litem->item = item;
		new_litem->lnext = NULL;

		pthread_mutex_lock(&clist->lock);
			if(clist->tail == NULL){
				clist->head = new_litem;
			}else{
				clist->tail->lnext = new_litem;
			}
			clist->tail = new_litem;	
		pthread_mutex_unlock(&clist->lock);
	}
}

void * list_pop(list *clist){
	litem *poped_item;

	pthread_mutex_lock(&clist->lock);
		poped_item = clist->head;
		if(poped_item != NULL){
			clist->head = poped_item->lnext;
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
int list_unlink(list *clist, void *item){
	int ret = -EINVAL;

	pthread_mutex_lock(&clist->lock);
	
	litem *curr, *last;
	last = NULL;
	curr = clist->head;
	while(curr != NULL){
		if(curr->item == item){
			if(last != NULL)
				last->lnext = curr->lnext;
			if(clist->tail == curr)
				clist->tail = last;
			if(clist->head == curr)
				clist->head = curr->lnext;
			free(curr);
			ret = 0;
			break;
		}
		last = curr;
		curr = curr->lnext;
	}
	
	pthread_mutex_unlock(&clist->lock);
	
	return ret;
}
*/

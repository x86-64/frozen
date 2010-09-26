#ifndef BACKEND_H
#define BACKEND_H

/* chains */

typedef struct chain_t    chain_t;
typedef enum chain_types  chain_types;

typedef int (*f_init)      (chain_t *);
typedef int (*f_configure) (chain_t *, setting_t *);
typedef int (*f_destroy)   (chain_t *);


enum chain_types {
	CHAIN_TYPE_CRWD
	
};

/* chain type crwd */

typedef int (*f_crwd_create) (chain_t *, void *, size_t);
typedef int (*f_crwd_set)    (chain_t *, void *, buffer_t *, size_t);
typedef int (*f_crwd_get)    (chain_t *, void *, buffer_t *, size_t);
typedef int (*f_crwd_delete) (chain_t *, void *, size_t);
typedef int (*f_crwd_move)   (chain_t *, void *, void *, size_t);
typedef int (*f_crwd_count)  (chain_t *, void *);

struct chain_t {
	char *      name;
	chain_types type;
	
	f_init      func_init;
	f_configure func_configure;
	f_destroy   func_destroy;
	union {
		struct {
			f_crwd_create func_create;
			f_crwd_set    func_set;
			f_crwd_get    func_get;
			f_crwd_delete func_delete;
			f_crwd_move   func_move;
			f_crwd_count  func_count;
		} chain_type_crwd;
	};
	
	chain_t *   next;
	void *      user_data;
}; 

#define CHAIN_REGISTER(chain_info) \
	static void __attribute__((constructor))__chain_init_##chain_info() \
	{ \
		_chain_register(&chain_info); \
	}

void        _chain_register    (chain_t *chain);
chain_t *   chain_new          (char *name);
void        chain_destroy      (chain_t *chain);
_inline int chain_configure    (chain_t *chain, setting_t *setting){ return chain->func_configure(chain, setting); }

/* chain crwd curr {{{1 */
_inline int chain_crwd_create (chain_t *chain, void *key, size_t value_size){
	return chain->chain_type_crwd.func_create(chain, key, value_size);
}
_inline int chain_crwd_set    (chain_t *chain, void *key, buffer_t *value, size_t value_size){
	return chain->chain_type_crwd.func_set(chain, key, value, value_size);
}
_inline int chain_crwd_get    (chain_t *chain, void *key, buffer_t *value, size_t value_size){
	return chain->chain_type_crwd.func_get(chain, key, value, value_size);
}
_inline int chain_crwd_delete (chain_t *chain, void *key, size_t value_size){
	return chain->chain_type_crwd.func_delete(chain, key, value_size);
}
_inline int chain_crwd_move   (chain_t *chain, void *key_from, void *key_to, size_t value_size){
	return chain->chain_type_crwd.func_move(chain, key_from, key_to, value_size);
}
_inline int chain_crwd_count  (chain_t *chain, void *count){
	return chain->chain_type_crwd.func_count(chain, count);
}
/* }}}1 */
/* chain crwd next {{{1 */
_inline int chain_next_crwd_create (chain_t *chain, void *key, size_t value_size){
	if(chain->next == NULL)
		return -EINVAL;
	
	return chain->next->chain_type_crwd.func_create(chain->next, key, value_size);
}
_inline int chain_next_crwd_set    (chain_t *chain, void *key, buffer_t *value, size_t value_size){
	if(chain->next == NULL)
		return -EINVAL;
	
	return chain->next->chain_type_crwd.func_set(chain->next, key, value, value_size);
}
_inline int chain_next_crwd_get    (chain_t *chain, void *key, buffer_t *value, size_t value_size){
	if(chain->next == NULL)
		return -EINVAL;
	
	return chain->next->chain_type_crwd.func_get(chain->next, key, value, value_size);
}
_inline int chain_next_crwd_delete (chain_t *chain, void *key, size_t value_size){
	if(chain->next == NULL)
		return -EINVAL;
	
	return chain->next->chain_type_crwd.func_delete(chain->next, key, value_size);
}
_inline int chain_next_crwd_move   (chain_t *chain, void *key_from, void *key_to, size_t value_size){
	if(chain->next == NULL)
		return -EINVAL;
	
	return chain->next->chain_type_crwd.func_move(chain->next, key_from, key_to, value_size);
}
_inline int chain_next_crwd_count  (chain_t *chain, void *count){
	if(chain->next == NULL)
		return -EINVAL;
	
	return chain->next->chain_type_crwd.func_count(chain->next, count);
}
/* }}}1 */

/* backends */

typedef struct backend_t {
	char *    name;
	chain_t * chain;
	
} backend_t;

backend_t *  backend_new          (char *name, setting_t *setting);
void         backend_destory      (backend_t *backend);
backend_t *  backend_find_by_name (char *name);

/* backend crwd {{{1 */
_inline int backend_crwd_create (backend_t *backend, void *key, size_t value_size){
	return backend->chain->chain_type_crwd.func_create(backend->chain, key, value_size);
}
_inline int backend_crwd_set    (backend_t *backend, void *key, buffer_t *value, size_t value_size){
	return backend->chain->chain_type_crwd.func_set(backend->chain, key, value, value_size);
}
_inline int backend_crwd_get    (backend_t *backend, void *key, buffer_t *value, size_t value_size){
	return backend->chain->chain_type_crwd.func_get(backend->chain, key, value, value_size);
}
_inline int backend_crwd_delete (backend_t *backend, void *key, size_t value_size){
	return backend->chain->chain_type_crwd.func_delete(backend->chain, key, value_size);
}
_inline int backend_crwd_move   (backend_t *backend, void *key_from, void *key_to, size_t value_size){
	return backend->chain->chain_type_crwd.func_move(backend->chain, key_from, key_to, value_size);
}
_inline int backend_crwd_count  (backend_t *backend, void *count){
	return backend->chain->chain_type_crwd.func_count(backend->chain, count);
}
/* }}}1 */

#endif // BACKEND_H

#ifndef BACKEND_H
#define BACKEND_H

enum request_actions {
	ACTION_CRWD_CREATE = 1,
	ACTION_CRWD_READ = 2,
	ACTION_CRWD_WRITE = 4,
	ACTION_CRWD_DELETE = 8,
	ACTION_CRWD_MOVE = 16,
	ACTION_CRWD_COUNT = 32,
	ACTION_CRWD_CUSTOM = 64,
	
	REQUEST_INVALID = 0
};

/* PRIVATE - chains {{{1 */


typedef int (*f_init)      (chain_t *);
typedef int (*f_configure) (chain_t *, hash_t *);
typedef int (*f_destroy)   (chain_t *);

typedef enum chain_types {
	CHAIN_TYPE_CRWD
	
} chain_types;

/* chain type crwd */

typedef ssize_t (*f_crwd) (chain_t *, request_t *);

struct chain_t {
	char *      name;
	chain_types type;
	
	f_init      func_init;
	f_configure func_configure;
	f_destroy   func_destroy;
	union {
		struct {
			f_crwd  func_create;
			f_crwd  func_set;
			f_crwd  func_get;
			f_crwd  func_delete;
			f_crwd  func_move;
			f_crwd  func_count;
			f_crwd  func_custom;
		} chain_type_crwd;
	};
	
	chain_t *   next;
	void *      userdata;
}; 

#define CHAIN_REGISTER(chain_info) \
	static void __attribute__((constructor))__chain_init_##chain_info() \
	{ \
		_chain_register(&chain_info); \
	}

void        _chain_register    (chain_t *chain);
chain_t *   chain_new          (char *name);
ssize_t     chain_query        (chain_t *chain, request_t *request);
ssize_t     chain_next_query   (chain_t *chain, request_t *request);
void        chain_destroy      (chain_t *chain);
_inline int chain_configure    (chain_t *chain, hash_t *config);

/* }}}1 */

/* backends {{{ */

typedef struct backend_t {
	chain_t        *chain;
	data_t          name;
	unsigned int    refs;
} backend_t;

API backend_t *     backend_new             (hash_t *config);
API ssize_t         backend_bulk_new        (hash_t *config);
API backend_t *     backend_acquire         (data_t *name);
API backend_t *     backend_find            (data_t *name);
API ssize_t         backend_query           (backend_t *backend, request_t *request);
API void            backend_destroy         (backend_t *backend);

API char *          backend_get_name        (backend_t *backend);

API void            backend_buffer_io_init  (buffer_t *buffer, chain_t *chain, int cached);
API buffer_t *      backend_buffer_io_alloc (chain_t *chain, int cached);

/* }}} */

request_actions     request_str_to_action   (char *string);

#endif // BACKEND_H

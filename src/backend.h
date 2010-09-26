#ifndef BACKEND_H
#define BACKEND_H

/* PRIVATE - chains {{{1 */

typedef struct chain_t    chain_t;

typedef int (*f_init)      (chain_t *);
typedef int (*f_configure) (chain_t *, setting_t *);
typedef int (*f_destroy)   (chain_t *);

typedef enum chain_types {
	CHAIN_TYPE_CRWD
	
} chain_types;

/* chain type crwd */

typedef ssize_t (*f_crwd) (chain_t *, request_t *, buffer_t *);

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
ssize_t     chain_query        (chain_t *chain, request_t *request, buffer_t *buffer);
ssize_t     chain_next_query   (chain_t *chain, request_t *request, buffer_t *buffer);
void        chain_destroy      (chain_t *chain);
_inline int chain_configure    (chain_t *chain, setting_t *setting){ return chain->func_configure(chain, setting); }

/* }}}1 */

/* PRIVATE - backends {{{1 */

typedef struct backend_t {
	char *    name;
	chain_t * chain;
	
} backend_t;

backend_t *  backend_new          (char *name, setting_t *setting);
ssize_t      backend_query        (backend_t *backend, request_t *request, buffer_t *buffer);
void         backend_destory      (backend_t *backend);
backend_t *  backend_find_by_name (char *name);

/* }}}1 */

/* fastcalls {{{ */
typedef struct crwd_fastcall {
	request_t    *request_create;
	request_t    *request_read;
	request_t    *request_write;
	request_t    *request_move;
	request_t    *request_delete;
	request_t    *request_count;
	
	buffer_t     *buffer_create;
	buffer_t     *buffer_read;
	buffer_t     *buffer_write;
	buffer_t     *buffer_count;
	
} crwd_fastcall;

int             fc_crwd_init       (crwd_fastcall *fc_table);
void            fc_crwd_destory (crwd_fastcall *fc_table);
ssize_t         fc_crwd_chain      (crwd_fastcall *fc_table, ...);
ssize_t         fc_crwd_next_chain (crwd_fastcall *fc_table, ...);
ssize_t         fc_crwd_backend    (crwd_fastcall *fc_table, ...);
/* }}} */

#endif // BACKEND_H

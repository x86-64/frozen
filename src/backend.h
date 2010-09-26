/* chains */

extern list  chains;

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
typedef int (*f_crwd_set)    (chain_t *, void *, void *, size_t);
typedef int (*f_crwd_get)    (chain_t *, void *, void *, size_t);
typedef int (*f_crwd_delete) (chain_t *, void *, size_t);

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
		} chain_type_crwd;
	};
	
	chain_t *   next;
	void *      user_data;
}; 


#define CHAIN_REGISTER(chain_info) \
	static void __attribute__((constructor))__chain_init_##chain_info() \
	{ \
		list_push(&chains, &chain_info); \
	}


chain_t *  chain_new          (char *name);
void       chain_destroy      (chain_t *chain);
int        chain_configure    (chain_t *chain, setting_t *setting);

/* backends */

extern list  backends;

typedef struct backend_t {
	char *    name;
	chain_t * chain;
	
} backend_t;

backend_t *  backend_new          (char *name, setting_t *setting);
void         backend_destory      (backend_t *backend);
backend_t *  backend_find_by_name (char *name);


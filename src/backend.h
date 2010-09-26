
typedef struct backend_ {
	char *  name;
	
} backend;




/* chains */

extern list  chains;

typedef struct chain_t    chain_t;
typedef enum chain_types  chain_types;

typedef int (*f_init)      (chain_t *);
typedef int (*f_configure) (chain_t *, void *);
typedef int (*f_destroy)   (chain_t *);


enum chain_types {
	CHAIN_TYPE_RWD
	
};

/* chain type rwd */
typedef int (*f_rwd_set)    (void *, void *);
typedef int (*f_rwd_get)    (void *, void *);
typedef int (*f_rwd_delete) (void *, void *);


struct chain_t {
	char *      name;
	chain_types type;
	
	f_init      func_init;
	f_configure func_configure;
	f_destroy   func_destroy;
	union {
		struct {
			f_rwd_set    func_set;
			f_rwd_get    func_get;
			f_rwd_delete func_delete;
		} chain_type_rwd;
	} funcs;
	
	void *      user_data;
}; 


#define CHAIN_REGISTER(chain_info) \
	static void __attribute__((constructor))__chain_init_##chain_info() \
	{ \
		list_push(&chains, &chain_info); \
	}


#include <libfrozen.h>
#include <backend.h>

/* chains */
list   chains;

static void __attribute__ ((constructor)) __chain_init(){
	list_init(&chains);
}


chain_t * chain_new(void){
	chain_t *chain = (chain_t *)malloc(sizeof(chain));
	return chain;
}

void chain_free(chain_t *chain){
	free(chain);
}


#include <libfrozen.h>
#include <backend.h>

typedef struct file_user_data {
	char *    path;
	
} file_user_data;

static int file_init(chain_t *chain){
	file_user_data *user_data = malloc(sizeof(file_user_data));
	if(user_data == NULL){
		return -ENOMEM;
	}
	memset(user_data, 0, sizeof(file_user_data));
	
	chain->user_data = user_data;
	
	return 0;
}

static int file_destroy(chain_t *chain){
	free(chain->user_data);
	
	chain->user_data = NULL;
	
	return 0;
}

static int file_configure(chain_t *chain, void *config){
	
}

static int file_set(void *key, void *value){
	
}

static int file_get(void *key, void *value){
	
}

static int file_delete(void *key, void *value){
	
}

static chain_t chain_file = {
	"file",
	CHAIN_TYPE_RWD,
	&file_init,
	&file_configure,
	&file_destroy,
	{
		&file_set,
		&file_get,
		&file_delete
	}
};
CHAIN_REGISTER(chain_file)


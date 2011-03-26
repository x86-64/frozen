#include <libfrozen.h>
#include <hash_t.h>

static void   data_hash_t_free(data_t *data){
	if(data->data_size != 0)
		hash_free(data->data_ptr);
}

data_proto_t hash_t_proto = {
	.type                   = TYPE_HASHT,
	.type_str               = "hash_t",
	.size_type              = SIZE_VARIABLE,
	.func_free              = &data_hash_t_free
};

#include <libfrozen.h>
#include <hash_t.h>

static ssize_t data_hash_t_copy(data_t *dst, data_t *src){
	dst->type      = src->type;
	dst->data_size = src->data_size;
	if( (dst->data_ptr = hash_copy(src->data_ptr)) == NULL)
		return -EFAULT;
	
	return 0;
}
static void   data_hash_t_free(data_t *data){
	if(data->data_size != 0)
		hash_free(data->data_ptr);
}

data_proto_t hash_t_proto = {
	.type                   = TYPE_HASHT,
	.type_str               = "hash_t",
	.size_type              = SIZE_VARIABLE,
	.func_copy              = &data_hash_t_copy,
	.func_free              = &data_hash_t_free,
};

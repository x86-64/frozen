#include <libfrozen.h>

static void   data_hash_t_free(data_t *data){
	if(data->data_size != 0)
		hash_free(data->data_ptr);
}

/*
REGISTER_TYPE(TYPE_HASHT)
REGISTER_MACRO(`DATA_HASHT(...)',            `TYPE_HASHT, (hash_t []){ __VA_ARGS__ }, sizeof((hash_t []){ __VA_ARGS__ })')
REGISTER_MACRO(`DATA_PTR_HASHT(_hash)',      `TYPE_HASHT, (void *)_hash, 0')
REGISTER_MACRO(`DATA_PTR_HASHT_FREE(_hash)', `TYPE_HASHT, (void *)_hash, 1')
REGISTER_PROTO(
	`{
		.type                   = TYPE_HASHT,
		.type_str               = "hash_t",
		.size_type              = SIZE_VARIABLE,
		.func_free              = &data_hash_t_free
	}'
)
*/
/* vim: set filetype=c: */

#include <libfrozen.h>

/*
REGISTER_TYPE(TYPE_HASHT)
REGISTER_MACRO(`DATA_HASHT(...)',       `TYPE_HASHT, (hash_t []){ __VA_ARGS__ }, sizeof((hash_t []){ __VA_ARGS__ })')
REGISTER_MACRO(`DATA_PTR_HASHT(_hash)', `TYPE_HASHT, (void *)_hash, 0')
REGISTER_PROTO(
	`{
		.type                   = TYPE_HASHT,
		.type_str               = "hash_t",
		.size_type              = SIZE_VARIABLE
	}'
)
*/
/* vim: set filetype=c: */

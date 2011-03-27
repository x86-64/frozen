#ifndef DATA_HASH_T_H
#define DATA_HASH_T_H

enum { TYPE_HASHT = 17 };
#define DATA_HASHT(...)             { TYPE_HASHT, (hash_t []){ __VA_ARGS__ }, sizeof((hash_t []){ __VA_ARGS__ }) }
#define DATA_PTR_HASHT(_hash)       { TYPE_HASHT, (void *)_hash, 0 }
#define DATA_PTR_HASHT_FREE(_hash)  { TYPE_HASHT, (void *)_hash, 1 }
#define DT_HASHT                    hash_t *
#define GET_TYPE_HASHT(value)       (hash_t *)value->data_ptr

extern data_proto_t hash_t_proto;

#endif

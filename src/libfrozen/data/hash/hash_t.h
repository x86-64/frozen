#ifndef DATA_HASH_T_H
#define DATA_HASH_T_H

#define DATA_HASHT(...)             { TYPE_HASHT, (hash_t []){ __VA_ARGS__ } }
#define DATA_PTR_HASHT(_hash)       { TYPE_HASHT, (void *)_hash }

extern data_proto_t hash_t_proto;

#endif

#ifndef DATA_HASH_T_H
#define DATA_HASH_T_H

#define DATA_HASHT(...)             { TYPE_HASHT, (hash_t []){ __VA_ARGS__ } }
#define DATA_PTR_HASHT(_hash)       { TYPE_HASHT, (void *)_hash }
#define DEREF_TYPE_HASHT(_data) (hash_t *)((_data)->ptr)
#define REF_TYPE_HASHT(_dt) _dt
#define HAVEBUFF_TYPE_HASHT 0

#endif

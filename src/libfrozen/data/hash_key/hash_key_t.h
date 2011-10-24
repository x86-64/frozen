#ifndef DATA_HASHKEY_T_H
#define DATA_HASHKEY_T_H

#define DATA_HASHKEYT(...)  { TYPE_HASHKEYT, (hash_key_t []){ __VA_ARGS__ } }
#define DEREF_TYPE_HASHKEYT(_data) *(hash_key_t *)((_data)->ptr)
#define REF_TYPE_HASHKEYT(_dt) (&(_dt))
#define HAVEBUFF_TYPE_HASHKEYT 1

#endif

#ifndef HASH_H
#define HASH_H

/*
 * hash_t memory layout
 *  - n - elements count
 *
 *          0: size in bytes
 *          4: hash_t[0] 
 *       4+12: hash_t[1]
 *         ..: ...
 * 4+12*(n-1): hash_t[n-1]
 * 4+12*(n  ): hash_t - zeroed, checked by .key
 * 4+12*(n+1): data chunks
 *
 */

typedef struct hash_el_t {
	unsigned int    type;   // enum data_type
	unsigned int    key;    // char *
	unsigned int    value;  // int / ptr
} hash_el_t;

typedef struct hash_t {
	unsigned int    size;
	// hash_el_t here
} hash_t;

typedef struct hash_builder_t {
	size_t         	alloc_size;
	unsigned int    nelements;
	hash_t         *hash;
	char           *data_ptr;
	unsigned int    data_last_off;
	// hash_t here
} hash_builder_t;


hash_builder_t *   hash_builder_new             (unsigned int nelements);
int                hash_builder_add_data        (hash_builder_t **builder, char *key, data_type type, data_t *value);
hash_t *           hash_builder_get_hash        (hash_builder_t *builder);
void               hash_builder_free            (hash_builder_t *builder);

int                hash_audit                   (hash_t *hash, size_t buffer_size);

int                hash_is_valid_buf            (hash_t *hash, data_t *data, unsigned int size);
int                hash_get                     (hash_t *hash, char *key, data_type *type, data_t **data);
data_t *           hash_get_typed               (hash_t *hash, char *key, data_type  type);
int                hash_get_in_buf              (hash_t *hash, char *key, data_type  type, data_t *buf);

#endif // HASH_H

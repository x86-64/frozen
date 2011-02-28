%module Frozen
%{
#include "libfrozen.h"
%}

enum request_actions {
	ACTION_CRWD_CREATE = 1,
	ACTION_CRWD_READ = 2,
	ACTION_CRWD_WRITE = 4,
	ACTION_CRWD_DELETE = 8,
	ACTION_CRWD_MOVE = 16,
	ACTION_CRWD_COUNT = 32,
	ACTION_CRWD_CUSTOM = 64,
	
	REQUEST_INVALID = 0
};


int                frozen_init(void);
int                frozen_destroy(void);

backend_t *        backend_new                  (hash_t *config);
ssize_t            backend_bulk_new             (hash_t *config);
backend_t *        backend_acquire              (char *name);
backend_t *        backend_find                 (char *name);
ssize_t            backend_query                (backend_t *backend, request_t *request);
void               backend_destroy              (backend_t *backend);

hash_t *           configs_string_parse         (char *string);
hash_t *           configs_file_parse           (char *filename);

hash_t *           hash_new                     (size_t nelements);
hash_t *           hash_copy                    (hash_t *hash);
void               hash_free                    (hash_t *hash);

hash_t *           hash_find                    (hash_t *hash, hash_key_t key);
void               hash_chain                   (hash_t *hash, hash_t *hash_next);
size_t             hash_nelements               (hash_t *hash);

hash_key_t         hash_string_to_key           (char *string);
char *             hash_key_to_string           (hash_key_t key);
hash_key_t         hash_key_to_ctx_key          (hash_key_t key);

hash_key_t         hash_item_key                (hash_t *hash);
data_t *           hash_item_data               (hash_t *hash);
hash_t *           hash_item_next               (hash_t *hash);
void               hash_data_find               (hash_t *hash, hash_key_t key, data_t **data, data_ctx_t **data_ctx);


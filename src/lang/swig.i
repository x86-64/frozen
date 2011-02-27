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


int             frozen_init(void);
int             frozen_destroy(void);

backend_t *     backend_new             (hash_t *config);
ssize_t         backend_bulk_new        (hash_t *config);
backend_t *     backend_acquire         (char *name);
backend_t *     backend_find            (char *name);
ssize_t         backend_query           (backend_t *backend, request_t *request);
void            backend_destroy         (backend_t *backend);

hash_t *        configs_string_parse    (char *string);
hash_t *        configs_file_parse      (char *filename);
void            hash_free               (hash_t *hash);


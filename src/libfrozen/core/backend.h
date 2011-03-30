#ifndef BACKEND_H
#define BACKEND_H

#include <pthread.h>

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

typedef enum api_types {
	API_CRWD
	
} api_types;

typedef int     (*f_init)      (backend_t *);
typedef int     (*f_configure) (backend_t *, hash_t *);
typedef int     (*f_destroy)   (backend_t *);
typedef ssize_t (*f_crwd)      (backend_t *, request_t *);

struct backend_t {
	char                  *name;
	char                  *class;
	uintmax_t              refs;        // 1 - on creation; 2,3 and more - on links to childs or _acquire
	pthread_mutex_t        mutex;
	
	api_types              supported_api;
	f_init                 func_init;
	f_configure            func_configure;
	f_destroy              func_destroy;
	union {
		struct {
			f_crwd  func_create;
			f_crwd  func_set;
			f_crwd  func_get;
			f_crwd  func_delete;
			f_crwd  func_move;
			f_crwd  func_count;
			f_crwd  func_custom;
		} backend_type_crwd;
	};
	void *                 userdata;
	
	backend_t            **parents;
	backend_t            **childs;
};

API backend_t *     backend_new             (hash_t *config);
API ssize_t         backend_bulk_new        (hash_t *config);
API backend_t *     backend_acquire         (char *name);
API backend_t *     backend_find            (char *name);
API ssize_t         backend_query           (backend_t *backend, request_t *request);
    ssize_t         backend_pass            (backend_t *backend, request_t *request);
API void            backend_destroy         (backend_t *backend);
    void            backend_destroy_all     (void);

API char *          backend_get_name        (backend_t *backend);

API ssize_t         backend_stdcall_create  (backend_t *backend, off_t *offset, size_t size);
API ssize_t         backend_stdcall_read    (backend_t *backend, off_t  offset, void *buffer, size_t buffer_size);
API ssize_t         backend_stdcall_write   (backend_t *backend, off_t  offset, void *buffer, size_t buffer_size);
API ssize_t         backend_stdcall_fill    (backend_t *backend, off_t  offset, void *buffer, size_t buffer_size, size_t fill_size);
API ssize_t         backend_stdcall_delete  (backend_t *backend, off_t  offset, size_t size);
API ssize_t         backend_stdcall_count   (backend_t *backend, size_t *count);

    request_actions request_str_to_action   (char *string);

// TODO move to buffer_t.c
API void            backend_buffer_io_init  (buffer_t *buffer, backend_t *chain, int cached);
API buffer_t *      backend_buffer_io_alloc (backend_t *backend, int cached);

#endif // BACKEND_H

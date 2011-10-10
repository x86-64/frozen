#ifndef BACKEND_H
#define BACKEND_H

#include <pthread.h>

typedef enum api_types {
	API_HASH = 1,
	API_CRWD = 2,
	API_FAST = 4
} api_types;

typedef int     (*f_init)      (backend_t *);
typedef int     (*f_fork)      (backend_t *, backend_t *, hash_t *);
typedef int     (*f_configure) (backend_t *, hash_t *);
typedef int     (*f_destroy)   (backend_t *);
typedef ssize_t (*f_crwd)      (backend_t *, request_t *);

typedef ssize_t (*f_fast_func) (backend_t *, void *);

struct backend_t {
	char                  *name;
	char                  *class;
	
	api_types              supported_api;
	f_init                 func_init;
	f_configure            func_configure;
	f_fork                 func_fork;
	f_destroy              func_destroy;
	
	struct {
		f_crwd  func_create;
		f_crwd  func_set;
		f_crwd  func_get;
		f_crwd  func_delete;
		f_crwd  func_move;
		f_crwd  func_count;
		f_crwd  func_custom;
	} backend_type_crwd;
	struct {
		f_crwd         func_handler;
	} backend_type_hash;
	struct {
		f_fast_func    func_handler;
	} backend_type_fast;

	void *                 userdata;
	
	uintmax_t              refs;
	pthread_mutex_t        refs_mtx;
	config_t              *config;
	
	list                   parents; // parent backends including private _acquires
	list                   childs;  // child backends
};

API ssize_t         class_register          (backend_t *proto);
API void            class_unregister        (backend_t *proto);

API backend_t *     backend_new             (hash_t *config);
API backend_t *     backend_acquire         (char *name);
API backend_t *     backend_find            (char *name);
API void            backend_destroy         (backend_t *backend);

API backend_t *     backend_clone           (backend_t *backend);
API backend_t *     backend_fork            (backend_t *backend, request_t *request);

API ssize_t         backend_query           (backend_t *backend, request_t *request);
API ssize_t         backend_pass            (backend_t *backend, request_t *request);
API ssize_t         backend_fast_query      (backend_t *backend, void *args);
API ssize_t         backend_fast_pass       (backend_t *backend, void *args);

API void            backend_connect         (backend_t *parent, backend_t *child);
API void            backend_disconnect      (backend_t *parent, backend_t *child);
API void            backend_insert          (backend_t *parent, backend_t *new_child);
API void            backend_add_terminators (backend_t *backend, list *terminators);

     void           backend_destroy_all     (void);

    data_functions  request_str_to_action   (char *string);

#endif // BACKEND_H

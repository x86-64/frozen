#ifndef BACKEND_H
#define BACKEND_H

/** @file backend.h */

typedef int     (*f_init)      (backend_t *);                         ///< Init function for backend. Allocate userdata and init default values here.
typedef int     (*f_fork)      (backend_t *, backend_t *, hash_t *);  ///< Fork function for backend. Make fork of current backend here.
typedef int     (*f_configure) (backend_t *, hash_t *);               ///< Configure function for backend. Try to make it reentrant. Assign configuration values and validate it here.
typedef int     (*f_destroy)   (backend_t *);                         ///< Destroy function for backend. Free all allocated resources here.

/** @brief Backend structure
 *  
 *  Used in class descriptions and as structure for real backends
 */
struct backend_t {
	char                  *name;             ///< Current instance name. Must be unique. Optional
	char                  *class;            ///< Backend class. Format: "category/classname", for example "storage/file". Required.
	
	api_types              supported_api;    ///< Supported apis. For example (API_HASH | API_FAST). Required
	f_init                 func_init;        ///< Backend init function. Optional
	f_configure            func_configure;   ///< Backend configure function. Optional
	f_fork                 func_fork;        ///< Backend fork function. Optional
	f_destroy              func_destroy;     ///< Backend destroy function. Optional
	
	struct {      // TODO deprecate this
		f_crwd         func_create;
		f_crwd         func_set;
		f_crwd         func_get;
		f_crwd         func_delete;
		f_crwd         func_move;
		f_crwd         func_count;
		f_crwd         func_custom;
	} backend_type_crwd;
	struct {
		f_crwd         func_handler;     ///< Handler for API_HASH
	} backend_type_hash;
	struct {
		f_fast_func    func_handler;     ///< Handler for API_FAST
	} backend_type_fast;

	void *                 userdata;         ///< Pointer to userdata for backend
	config_t              *config;           ///< Current instance configuraion. (Copy of passed to backend_new)

	uintmax_t              refs;             ///< Internal.
	list                   parents;          ///< parent backends including private _acquires. Internal.
	list                   childs;           ///< child backends. Internal.
};

API ssize_t         class_register          (backend_t *proto); ///< Register new dynamic class
API void            class_unregister        (backend_t *proto); ///< Unregister dynamic class

/** @brief Create new backend.
 *
 *  Configuration is hash with items in reverse order. For example:
 *  @code
 *       hash_t example_cfg[] = {
 *             { 0, DATA_HASHT(
 *                   { HK(class), DATA_STRING("storage/file") },
 *                   hash_end
 *             )},
 *             { 0, DATA_HASHT(
 *                   { HK(class), DATA_STRING("request/debug") },
 *                   hash_end
 *             )},
 *             hash_end
 *       };
 *  @endcode
 *  This config produce two backends: debug and file. Debug will have one child - file and will pass
 *  all requests to it. backend_new will return pointer to last created backend. In this case - this is debug.
 *  
 *  @li Use hash_null to avoid linkage between backend.
 *  @retval NULL     Creation failed
 *  @retval non-NULL Creation success
 */
API backend_t *     backend_new             (hash_t *config);
API backend_t *     backend_acquire         (char *name); ///< Find backend by name and increment ref counter. Use backend_destroy to free.
API backend_t *     backend_find            (char *name); ///< Find backend by name.
API void            backend_destroy         (backend_t *backend); ///< Destroy backend

/** @brief Clone backend
 *
 *  This function will create clone of supplied backend. Clone will not have links with any of backends.
 *  But it will have same pointer to userdata. No func_* functions will be called for it.
 *  
 *  @li Can be useful to make dummy backends, without full implementation with registration and classes.
 */
API backend_t *     backend_clone           (backend_t *backend);

/** @brief Fork whole chain of backends
 * 
 *  This function forks backends starting from suppliend backend. For each forked backend
 *  special function func_fork called, if supplied, or used ordinary configure.
 *
 *  @param backend  Backend to fork
 *  @param request  Fork request. Useful for passing some real-time configurations, like part of filename to open
 *  @retval NULL Fork failed
 *  @retval non-NULL Fork successfull
 */
API backend_t *     backend_fork            (backend_t *backend, request_t *request);

API ssize_t         backend_query           (backend_t *backend, request_t *request); ///< Query backend with hash request
API ssize_t         backend_fast_query      (backend_t *backend, void *args); ///< Query backend with fast request

API ssize_t         backend_pass            (backend_t *backend, request_t *request); ///< Pass hash request to next backends in chain. @param backend Current backend
API ssize_t         backend_fast_pass       (backend_t *backend, void *args); ///< Pass fast request to next backends in chain. @param backend Current backend

API void            backend_connect         (backend_t *parent, backend_t *child); ///< Create link between parent and child
API void            backend_disconnect      (backend_t *parent, backend_t *child); ///< Remove link between parent and child
API void            backend_insert          (backend_t *parent, backend_t *new_child); ///< Insert new_child in between of backend and it's childs
API void            backend_add_terminators (backend_t *backend, list *terminators); ///< Wrap backend with terminators.

     void           backend_destroy_all     (void);

    data_functions  request_str_to_action   (char *string);

#endif // BACKEND_H

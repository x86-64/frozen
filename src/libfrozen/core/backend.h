#ifndef BACKEND_H
#define BACKEND_H

/** @file backend.h */

/** @ingroup backend
 *  @page backend_overview Backends overview
 *  
 *  Every user request processed by one backend, or by backend chain. Backend can perform
 *  various actions:
 *    @li interact with external things, such as files, sockets, etc
 *    @li interact with internal things, with another backends, data
 *    @li can read, write and modify request's values
 *
 *  If user request is too complex for one backend, it can be processed with chain of backends.
 *  Chain of backends process user request step by step. One backend in such chain can modify
 *  request in way another backend can understand it. Another backends can do, as we already know,
 *  external things, for example write user data to file.
 *  
 *  There is several types of requests and corresponding apis. Details described in api section.
 *  
 */
/** @ingroup backend
 *  @page backend_creation Creation and configuration
 *  
 *  To create new backend you have to know it's "class name". For example - "storage/file". It
 *  consist of category name (storage) and backend name (file). Categories introduced to group
 *  identical backends to one place. There is serveral important categories: request, storage,
 *  data, io. These categories represent most common used operations with user requests.
 *    @li request - modify, print, user request; change request path
 *    @li storage - save and load user data from external storage
 *    @li data - modify, pack/unpack, regexp user data from request
 *    @li io - input and output from/to frozen, for example, fuse as filesystem for io
 *  There is more categories which is not listed.
 *
 *  Almost every backend need some configuration variables to control it's behavior. They can 
 *  be passed on new backend creation.
 *  
 *  Currently you can't change configuration passed on step of creation, but after rewriting
 *  some of backends it can be possible. Example of correct configuration routines is backend "backend/factory".
 *  So, if func_configure of backend is reentrant and updates taken in account - you can change configuration
 *  in any time.
 */
/** @ingroup backend
 *  @page backend_requests Requests
 *  
 *  Then user request arrived, backend's handler is called. Backend can:
 *    @li return error or success code
 *    @li read user request, check inputs, do something with it
 *    @li pass user request to next backend(s) in chain
 *    @li pass any request to next backend(s) in chain
 *    @li etc...
 *  
 *  All of these actions is optional, even returning error or success. There is special error code -EEXIST, which indicates
 *  that backend don't want to modify current error code.
 *  
 *  Internally all requests flow into some kind of recursive processing there each step is next backend. So,
 *  there is no such thing as async requests. Request considered finished then first backend return control to caller. But,
 *  async requests is too good to discard them, so you can use queue to save all requests and process them later, returning
 *  control almost immediatly. Of course, in such case, you should keep all data used in this request until this request
 *  will be really finished.
 */
/** @ingroup backend
 *  @page backend_childs Parents and childs
 *  
 *  Every backend, if this is not standalone backend, can have their parent(s) and child(s). Parent backend can pass to your backend
 *  requests. Childs is backends to which your backend can pass request. If backend have more than one child, request passed to every
 *  child repeatedly. If this is not that you want - you can solve it with any of path rewriting backend. Such approach is useful,
 *  because you can add child to any backend and monitor requests passed by. This can be used in logging.
 *  
 *  You can connect backends in any way you want, but there is one limit - if your configuration produce infinity loops they will not
 *  be terminated or avoided. However this is doesn't mean that you can't send requests to parent backends. Just make sure it will work.
 * 
 */


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
API void            backend_acquire         (backend_t *backend); ///< Increment ref counter of backend. Use backend_destroy to free.
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

    ssize_t         frozen_backend_init     (void);
    void            frozen_backend_destroy  (void);

    data_functions  request_str_to_action   (char *string);

extern pthread_mutex_t                destroy_mtx; // TODO remove this..

// Thread-specific userdata functions
typedef void   * (*f_thread_create)    (void);          ///< Thread-specific data constructor
typedef void     (*f_thread_destroy)   (void *);        ///< Thread-specific data destructor

/// Thread data context
typedef struct thread_data_ctx_t {
	uintmax_t              inited;                  ///< Inited structure or not
	pthread_key_t          key;                     ///< Pthread key to hold thread-specific data in it
	f_thread_create        func_create;             ///< Constructor function
} thread_data_ctx_t;

/** Initialize context for thread-specific data
 *  @param thread_data Context. Prefer to store it in backend's userdata
 *  @param func_create  Constructor function for thread-specific data
 *  @param func_destroy Destructor function for thread-specific data
 *  @retval 0 No errors
 *  @retval <0 Error occurred
 */
ssize_t            thread_data_init(thread_data_ctx_t *thread_data, f_thread_create func_create, f_thread_destroy func_destroy);

/** Destroy context for thread-specific data
 *  @param thread_data Context. Prefer to store it in backend's userdata
 */
void               thread_data_destroy(thread_data_ctx_t *thread_data);

/** Get thread-specific data
 *  @param thread_data Context. Prefer to store it in backend's userdata
 *  @retval ptr Anything that func_create returns
 */
void *             thread_data_get(thread_data_ctx_t *thread_data);

#endif // BACKEND_H

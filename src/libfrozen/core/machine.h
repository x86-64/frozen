#ifndef MACHINE_H
#define MACHINE_H

/** @file machine.h */

/** @ingroup machine
 *  @page machine_overview Machines overview
 *  
 *  Every user request processed by one machine, or by machine shop. Machine can perform
 *  various actions:
 *    @li interact with external things, such as files, sockets, etc
 *    @li interact with internal things, with another machines, data
 *    @li can read, write and modify request's values
 *
 *  If user request is too complex for one machine, it can be processed with shop of machines.
 *  Shop of machines process user request step by step. One machine in such shop can modify
 *  request in way another machine can understand it. Another machines can do, as we already know,
 *  external things, for example write user data to file.
 *  
 *  There is several types of requests and corresponding apis. Details described in api section.
 *  
 */
/** @ingroup machine
 *  @page machine_creation Creation and configuration
 *  
 *  To create new machine you have to know it's "class name". For example - "storage/file". It
 *  consist of category name (storage) and machine name (file). Categories introduced to group
 *  identical machines to one place. There is serveral important categories: request, storage,
 *  data, io. These categories represent most common used operations with user requests.
 *    @li request - modify, print, user request; change request path
 *    @li storage - save and load user data from external storage
 *    @li data - modify, pack/unpack, regexp user data from request
 *    @li io - input and output from/to frozen, for example, fuse as filesystem for io
 *  There is more categories which is not listed.
 *
 *  Almost every machine need some configuration variables to control it's behavior. They can 
 *  be passed on new machine creation.
 *  
 *  Currently you can't change configuration passed on step of creation, but after rewriting
 *  some of machines it can be possible. Example of correct configuration routines is machine "machine/factory".
 *  So, if func_configure of machine is reentrant and updates taken in account - you can change configuration
 *  in any time.
 */
/** @ingroup machine
 *  @page machine_requests Requests
 *  
 *  Then user request arrived, machine's handler is called. Machine can:
 *    @li return error or success code
 *    @li read user request, check inputs, do something with it
 *    @li pass user request to next machine(s) in shop
 *    @li pass any request to next machine(s) in shop
 *    @li etc...
 *  
 *  All of these actions is optional, even returning error or success. There is special error code -EEXIST, which indicates
 *  that machine don't want to modify current error code.
 *  
 *  Internally all requests flow into some kind of recursive processing there each step is next machine. So,
 *  there is no such thing as async requests. Request considered finished then first machine return control to caller. But,
 *  async requests is too good to discard them, so you can use queue to save all requests and process them later, returning
 *  control almost immediatly. Of course, in such case, you should keep all data used in this request until this request
 *  will be really finished.
 */
/** @ingroup machine
 *  @page machine_childs Parents and childs
 *  
 *  Every machine, if this is not standalone machine, can have their parent(s) and child(s). Parent machine can pass to your machine
 *  requests. Childs is machines to which your machine can pass request. If machine have more than one child, request passed to every
 *  child repeatedly. If this is not that you want - you can solve it with any of path rewriting machine. Such approach is useful,
 *  because you can add child to any machine and monitor requests passed by. This can be used in logging.
 *  
 *  You can connect machines in any way you want, but there is one limit - if your configuration produce infinity loops they will not
 *  be terminated or avoided. However this is doesn't mean that you can't send requests to parent machines. Just make sure it will work.
 * 
 */


typedef int     (*f_init)      (machine_t *);                         ///< Init function for machine. Allocate userdata and init default values here.
typedef int     (*f_fork)      (machine_t *, machine_t *, hash_t *);  ///< Fork function for machine. Make fork of current machine here.
typedef int     (*f_configure) (machine_t *, hash_t *);               ///< Configure function for machine. Try to make it reentrant. Assign configuration values and validate it here.
typedef int     (*f_destroy)   (machine_t *);                         ///< Destroy function for machine. Free all allocated resources here.

/** @brief Machine structure
 *  
 *  Used in class descriptions and as structure for real machines
 */
struct machine_t {
	char                  *name;             ///< Current instance name. Must be unique. Optional
	char                  *class;            ///< Machine class. Format: "category/classname", for example "storage/file". Required.
	
	api_types              supported_api;    ///< Supported apis. For example (API_HASH | API_FAST). Required
	f_init                 func_init;        ///< Machine init function. Optional
	f_configure            func_configure;   ///< Machine configure function. Optional
	f_fork                 func_fork;        ///< Machine fork function. Optional
	f_destroy              func_destroy;     ///< Machine destroy function. Optional
	
	struct {      // TODO deprecate this
		f_crwd         func_create;
		f_crwd         func_set;
		f_crwd         func_get;
		f_crwd         func_delete;
		f_crwd         func_move;
		f_crwd         func_count;
		f_crwd         func_custom;
	} machine_type_crwd;
	struct {
		f_crwd         func_handler;     ///< Handler for API_HASH
	} machine_type_hash;
	struct {
		f_fast_func    func_handler;     ///< Handler for API_FAST
	} machine_type_fast;

	void *                 userdata;         ///< Pointer to userdata for machine
	config_t              *config;           ///< Current instance configuraion. (Copy of passed to machine_new)

	uintmax_t              refs;             ///< Internal.
	machine_t             *cnext;            ///< Next machine in shop
	machine_t             *cprev;            ///< Prev machine in shop
};

API ssize_t         class_register          (machine_t *proto); ///< Register new dynamic class
API void            class_unregister        (machine_t *proto); ///< Unregister dynamic class

API request_t *     request_get_current(void); ///< Get current request

API ssize_t         machine_new             (machine_t **pmachine, hash_t *config); ///< Create machine
API void            machine_acquire         (machine_t *machine); ///< Increment ref counter of machine. Use shop_destroy to free.
API machine_t *     machine_find            (char *name); ///< Find machine by name and acquire it, if not found - create "ghost" machine
API void            machine_destroy         (machine_t *machine); ///< Destroy machine
API ssize_t         machine_is_ghost        (machine_t *machine);

API ssize_t         machine_query           (machine_t *machine, request_t *request); ///< Query machine with hash request
API ssize_t         machine_pass            (machine_t *machine, request_t *request); ///< Pass hash request to next machines in shop. @param machine Current machine

/** @brief Create new shop.
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
 *  This config produce two machines: debug and file. Debug will have one child - file and will pass
 *  all requests to it. machine_new will return pointer to last created machine. In this case - this is debug.
 *  
 *  @li Use hash_null to avoid linkage between machine.
 *  @retval NULL     Creation failed
 *  @retval non-NULL Creation success
 */
API machine_t *     shop_new               (hash_t *config);
API void            shop_destroy           (machine_t *machine); ///< Destroy shop

    ssize_t         frozen_machine_init     (void);
    void            frozen_machine_destroy  (void);

extern pthread_mutex_t                destroy_mtx; // TODO remove this..

// Thread-specific userdata functions
typedef void   * (*f_thread_create)    (void *);        ///< Thread-specific data constructor
typedef void     (*f_thread_destroy)   (void *);        ///< Thread-specific data destructor

/// Thread data context
typedef struct thread_data_ctx_t {
	uintmax_t              inited;                  ///< Inited structure or not
	pthread_key_t          key;                     ///< Pthread key to hold thread-specific data in it
	f_thread_create        func_create;             ///< Constructor function
	void                  *userdata;                ///< User data
} thread_data_ctx_t;

/** Initialize context for thread-specific data
 *  @param thread_data Context. Prefer to data it in machine's userdata
 *  @param func_create  Constructor function for thread-specific data
 *  @param func_destroy Destructor function for thread-specific data
 *  @param userdata User data
 *  @retval 0 No errors
 *  @retval <0 Error occurred
 */
ssize_t            thread_data_init(thread_data_ctx_t *thread_data, f_thread_create func_create, f_thread_destroy func_destroy, void *userdata);

/** Destroy context for thread-specific data
 *  @param thread_data Context. Prefer to data it in machine's userdata
 */
void               thread_data_destroy(thread_data_ctx_t *thread_data);

/** Get thread-specific data
 *  @param thread_data Context. Prefer to data it in machine's userdata
 *  @retval ptr Anything that func_create returns
 */
void *             thread_data_get(thread_data_ctx_t *thread_data);

#endif // MACHINE_H

#ifndef API_H
#define API_H

/** @file api.h */

/** @ingroup api
 *  @page api_overview Api overview
 *  
 *  Frozen currently have several api. One of the most important is API_HASH and API_FAST.
 *  They represent two different approaches to sending requests.
 *
 *  Then user want api he expect that api would be customizable and very fast. Both of these
 *  characteristics hardly can be implemented in one api set. So, in frozen, we introduce two
 *  api sets: API_HASH - for custom requests, and API_FAST for very fast requests.
 *
 *  Despite of kind of api used, every api deals with requests. Request is a set of key-value pairs.
 *
 *  Each request have "action" key to tell machine what to do with data. Some machines don't require
 *  action for request, so initial user request can have no action, but for further processing one of machines
 *  have to define it.
 */
/** @ingroup api
 *  @page api_hash API_HASH
 *  
 *  Hash api deals with specially structured key-value pairs. From the C point of view they are arrays
 *  of hash_t structure. To declare new hash you could write:
 *  @code
 *      hash_t new_hash[] = {
 *         { HK(key), DATA_UINTT(100) },
 *         hash_end
 *      };
 *      machine_query(machine, new_hash);
 *  @endcode
 *  This hash declare one parameter named "key" with value of 100.
 *
 *  Such structures used to describe user request and process it. It can hold any amount of parameters with
 *  any data type and any value. Also, data value isn't copied to hash, hash only hold pointer to data. So,
 *  unnessesary copying form place to place avoided.
 *
 *  If hash defined within function such declaration converted to several "mov" and "lea" assembly command,
 *  which write hash in current stack frame. This can be processed very fast, because of lack of shoped
 *  computation and hopefuly with help of caches.
 *
 *  Staticly declared hashes already datad in usable form, so no overhead here.
 *
 *  Hash can contain inline hashes, can contain embeded hashes and so on. @see hashes
 *
 *  In conclusion, advantages of this api type:
 *  @li Any number and order of parameters
 *  @li Any number of optional and required parameters, again, in any order
 *  @li Data not copied from user's buffer
 *  @li Fast "allocation" and assignment
 *
 *  Disadvantages:
 *  @li Very greedy to stack space. (However stack space is already allocated and used, so why not?)
 *  @li Key find is slow. This done by incremental search througth all parameters.
 *
 */
/** @ingroup api
 *  @page api_fast API_FAST
 *  
 *  As opposite to API_HASH, API_FAST was introduced. Main reason is speed - hash api is too slow
 *  for key find. Then some "index" machines deals with "memory" each user request produce hundreds of
 *  requests to "memory" machine, and every request to "memory" have at least 3 parameters
 *  (offset, size, buffer). So, hash_find api gets very busy and callgrind isn't very happy with that.
 *
 *  However, this situation is simple to solve. We know how many parameters we want to pass, and we have no
 *  optional parameters. Simpliest solution is pack all parameters in order known to both machines. In C world
 *  this can be perfectly done by defining a struct. So, we use them for fast api.
 *  
 *  Each action have own structure, where data defined in specified order. As bonus, you can have own
 *  optional paramenters by redefining structure.
 *
 *  Example:
 *  @code
 *       fastcall_read r_read = {
 *          { 5, ACTION_READ },         // { number of all arguments, action }
 *          0,                          // .offset
 *          &buffer,                    // .buffer
 *          100                         // .buffer_size
 *       };
 *       machine_fast_query(machine, &r_read);
 *  @endcode
 *
 *  Advantages:
 *  @li It is very likely that whole structure can fit in processor cache, so:
 *  @li Incredibly fast parameter search
 *  @li Still can have some optional parameters
 *
 *  Disadvantages:
 *  @li If you want change request action, you should copy old values and fill new structure
 *  @li If you want to change value of request and keep old value, you should keep value, or make new structure and fill it
 *  @li No freedom in parameter defines
 */
/** @ingroup api
 *  @page api_crwd API_CRWD
 *  
 *  Api crwd is same as api hash, but all requests splitted to flow into different handlers. This api would be transformed in
 *  same api as used in data processing. Don't use it for a while.
 */
/** @ingroup api
 *  @page api_downgrade Downgrading
 *  
 *  Currenly machine code have support for so called "request downgrading". This process occurs then some API_FAST-capable
 *  machine pass request to API_FAST-notcapable machine. So, this code creates new hash request and fill it with parameters from
 *  fast request.
 *
 *  This is very painful process. At first, all optional parameters is lost. Second, this is overhead in any case. Third,
 *  all further processing is done by hash apis. There is no such process as "upgrading" and never be.
 *
 *  As developer, try avoid this. As end user, you can ignore it.
 */
/** @ingroup api
 *  @page api_newmachine Recomendations for new machines
 *  
 *  If you write new machine you have to choose which api to implement.
 *
 *  If machine provide access to very fast things, such as memory - API_FAST is top priority.
 *  Common things, such as files, directories have to implement both API_HASH and API_FAST.
 *  Unusual things, and data processing can implement only API_HASH. 
 */

typedef enum api_types {
	API_HASH = 1,
	API_CRWD = 2,
	API_FAST = 4
} api_types;

typedef ssize_t (*f_crwd)      (machine_t *, request_t *);
typedef ssize_t (*f_fast_func) (data_t *,    void *);
typedef ssize_t (*f_callback)  (request_t *, void *);

/*
	ACTION(CREATE),
	ACTION(DELETE),
	ACTION(COUNT),
	ACTION(CUSTOM),
	ACTION(REBUILD),

	ACTION(EXECUTE),
	ACTION(START),
	ACTION(STOP),
	ACTION(ALLOC),
	ACTION(RESIZE),
	ACTION(FREE),
	ACTION(LENGTH),
	ACTION(COMPARE),
	ACTION(INCREMENT),
	ACTION(DECREMENT),
	ACTION(ADD),
	ACTION(SUB),
	ACTION(MULTIPLY),
	ACTION(DIVIDE),
	ACTION(READ),
	ACTION(WRITE),
	ACTION(CONVERT_TO)
	ACTION(CONVERT_FROM),
	ACTION(TRANSFER),
	ACTION(COPY),
	ACTION(IS_NULL),
	
	ACTION(PUSH),
	ACTION(POP),
	ACTION(ENUM),
	
	ACTION(QUERY),
	
	ACTION(GETDATAPTR),
	ACTION(GETDATA),
	
	ACTION(ACQUIRE)
*/

typedef struct fastcall_header {
	uintmax_t              nargs;
	uintmax_t              action;
} fastcall_header;

typedef struct fastcall_io {
	fastcall_header        header;
	uintmax_t              offset;
	void                  *buffer;
	uintmax_t              buffer_size;
} fastcall_io;
typedef struct fastcall_io     fastcall_read;
typedef struct fastcall_io     fastcall_write;

typedef struct fastcall_length {
	fastcall_header        header;
	uintmax_t              length;
	uintmax_t              format;
} fastcall_length;

typedef struct fastcall_alloc {
	fastcall_header        header;
	uintmax_t              length;
} fastcall_alloc;

typedef struct fastcall_resize {
	fastcall_header        header;
	uintmax_t              length;
} fastcall_resize;

typedef struct fastcall_free {
	fastcall_header        header;
} fastcall_free;

typedef struct fastcall_acquire {
	fastcall_header        header;
} fastcall_acquire;

typedef struct fastcall_execute {
	fastcall_header        header;
} fastcall_execute;
typedef struct fastcall_start {
	fastcall_header        header;
} fastcall_start;
typedef struct fastcall_stop {
	fastcall_header        header;
} fastcall_stop;

typedef struct fastcall_init {
	fastcall_header        header;
	char                  *string;
} fastcall_init;

typedef struct fastcall_copy {
	fastcall_header        header;
	data_t                *dest;
} fastcall_copy;

typedef struct fastcall_transfer {
	fastcall_header        header;
	data_t                *dest;
	uintmax_t              transfered;
} fastcall_transfer;

struct fastcall_convert_from {
	fastcall_header        header;
	data_t                *src;
	uintmax_t              format;
};
struct fastcall_convert_to {
	fastcall_header        header;
	data_t                *dest;
	uintmax_t              format;
	uintmax_t              transfered;
};
typedef struct fastcall_convert_to    fastcall_convert_to;
typedef struct fastcall_convert_from  fastcall_convert_from;

typedef struct fastcall_compare {
	fastcall_header        header;
	data_t                *data2;
	uintmax_t              result; // 0 - equal, 1 - data1 is less, 2 - data2 is less
} fastcall_compare;

typedef struct fastcall_arith {
	fastcall_header        header;
	data_t                *data2;
} fastcall_arith;
typedef struct fastcall_arith  fastcall_add;
typedef struct fastcall_arith  fastcall_sub;
typedef struct fastcall_arith  fastcall_mul;
typedef struct fastcall_arith  fastcall_div;

typedef struct fastcall_arith_no_arg {
	fastcall_header        header;
} fastcall_arith_no_arg;
typedef struct fastcall_arith_no_arg fastcall_increment;
typedef struct fastcall_arith_no_arg fastcall_decrement;

typedef struct fastcall_getdataptr {
	fastcall_header        header;
	void                  *ptr;
} fastcall_getdataptr;
typedef struct fastcall_getdata {
	fastcall_header        header;
	data_t                *data;
} fastcall_getdata;

typedef struct fastcall_is_null {
	fastcall_header        header;
	uintmax_t              is_null;
} fastcall_is_null;

typedef struct fastcall_create {
	fastcall_header        header;
	uintmax_t              size;
	uintmax_t              offset;
} fastcall_create;

typedef struct fastcall_delete {
	fastcall_header        header;
	uintmax_t              offset;
	uintmax_t              size;
} fastcall_delete;

typedef struct fastcall_count {
	fastcall_header        header;
	uintmax_t              nelements;
} fastcall_count;

typedef struct fastcall_push {
	fastcall_header        header;
	data_t                *data;
} fastcall_push;

typedef struct fastcall_pop {
	fastcall_header        header;
	data_t                *data;
} fastcall_pop;

typedef struct fastcall_enum {
	fastcall_header        header;
	data_t                *dest;
} fastcall_enum;

typedef struct fastcall_query {
	fastcall_header        header;
	request_t             *request;
} fastcall_query;

extern uintmax_t fastcall_nargs[];

ssize_t     data_hash_query(data_t *data, request_t *request);
ssize_t     machine_fast_query(machine_t *machine, void *hargs);

#endif

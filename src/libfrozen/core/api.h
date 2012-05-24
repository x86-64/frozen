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
 *  unnecessary copying form place to place avoided.
 *
 *  If hash defined within function such declaration converted to several "mov" and "lea" assembly command,
 *  which write hash in current stack frame. This can be processed very fast, because of lack of shoped
 *  computation and hopefully with help of caches.
 *
 *  Statically declared hashes already stored in usable form, so no overhead here.
 *
 *  Hash can contain inline hashes, can contain embedded hashes and so on. @see hashes
 *
 *  In conclusion, advantages of this api type:
 *  @li Any number and order of parameters
 *  @li Any number of optional and required parameters
 *  @li Data not copied from user's buffer
 *  @li Fast "allocation" and assignment
 *
 *  Disadvantages:
 *  @li Very greedy to stack space. (However stack space is already allocated and used, so why not?)
 *  @li Key find is slow. This done by incremental search through all parameters.
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
 *  optional parameters. Simplest solution is pack all parameters in order known to both machines. In C world
 *  this can be perfectly done by defining a struct. So, we use them for fast api.
 *  
 *  Each action have own structure, where data defined in specified order. As bonus, you can have own
 *  optional parameters by redefining structure.
 *
 *  Example:
 *  @code
 *       fastcall_read r_read = {
 *          { 5, ACTION_READ },         // { number of all arguments, action }
 *          0,                          // .offset
 *          &buffer,                    // .buffer
 *          100                         // .buffer_size
 *       };
 *       data_query(data, &r_read);
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

typedef enum api_types {
	API_HASH = 1,
	API_FAST = 4
} api_types;

typedef ssize_t (*f_hash)      (machine_t *, request_t *);
typedef ssize_t (*f_fast_func) (data_t *,    void *);
typedef ssize_t (*f_callback)  (request_t *, void *);

typedef struct fastcall_header {
	uintmax_t              nargs;
	uintmax_t              action;
} fastcall_header;

/*
	// Various
	ACTION(QUERY),
*/

// Core api set {{{
/** @ingroup api
 *  @addtogroup api_core Core api set
 */
/** @ingroup api_core
 *  @page api_core_overview Overview
 *  
 *  This is core data api set. It used for creation, transfer and free of data.
 */

/** @ingroup api_core
 *  @section api_core_convert_from Convert_from action
 *
 *  Used to transfer information from one data to another, this could be thought as "set" action. Destination data in this case
 *  updated or initialized from source. This function works from destination data point of view, so information should be acceptable for it.
 *
 *  <h3> For developer </h3>
 *  @li Handler should use basic storage api set for byte-array type of data to get it
 *  @li Function should count number of transfered bytes
 *  <h3> For user </h3>
 */
typedef struct fastcall_convert_from { // ACTION(CONVERT_FROM) 
	fastcall_header        header;
	data_t                *src;
	uintmax_t              format;
	uintmax_t              transfered;
} fastcall_convert_from;

/** @ingroup api_core
 *  @section api_core_convert_to Convert_to action
 *
 *  Used to transfer information from one data to another, this could be thought as "get" action. Destination data in this case
 *  updated or initialized from source. This function works from source data point of view.
 *
 *  <h3> For developer </h3>
 *  @li Handler should use basic storage api set for byte-array type of data to get it
 *  @li Function should count number of transfered bytes
 *  <h3> For user </h3>
 */
typedef struct fastcall_convert_to { // ACTION(CONVERT_TO) 
	fastcall_header        header;
	data_t                *dest;
	uintmax_t              format;
	uintmax_t              transfered;
} fastcall_convert_to;

/** @ingroup api_core
 *  @section api_core_free Free action
 *
 *  Used to free data and all related resources.
 *
 *  <h3> For developer </h3>
 *  @li Handler should free all allocated resources and call data_set_void on current data.
 *  <h3> For user </h3>
 */
typedef struct fastcall_free { // ACTION(FREE)
	fastcall_header        header;
} fastcall_free;

/** @ingroup api_core
 *  @section api_core_control Control action
 *
 *  Used to control datatype and request additional information about datatype.
 *
 *  <h3> For developer </h3>
 *  <h3> For user </h3>
 */
typedef struct fastcall_control { // ACTION(CONTROL)
	fastcall_header        header;
	uintmax_t              function;
	data_t                *key;
	data_t                *value;
} fastcall_control;

typedef struct fastcall_consume { // ACTION(CONSUME)
	fastcall_header        header;
	data_t                *dest;
} fastcall_consume;
// }}}
// Basic storage api set {{{
/** @ingroup api
 *  @addtogroup api_storage Basic storage api set
 */
/** @ingroup api_storage
 *  @page api_storage_overview Overview
 *  
 *  This is api set for byte-array storage. It used for allocation, free, read ans write for byte arrays.
 */

/** @ingroup api_storage
 *  @section api_storage_resize Resize action
 *
 *  Used to resize available space inside data.
 *
 *  <h3> For developer </h3>
 *  <h3> For user </h3>
 */
typedef struct fastcall_resize { // ACTION(RESIZE)
	fastcall_header        header;
	uintmax_t              length;
} fastcall_resize;

/** @ingroup api_storage
 *  @section api_storage_length Length measurement action
 *
 *  Used to get length of available space inside data.
 *
 *  <h3> For developer </h3>
 *  <h3> For user </h3>
 */
typedef struct fastcall_length { // ACTION(LENGTH)
	fastcall_header        header;
	uintmax_t              length;
	uintmax_t              format;
} fastcall_length;

/** @ingroup api_storage
 *  @section api_storage_read Read action
 *
 *  Used to read byte array from data info specified continuous buffer.
 *
 *  <h3> For developer </h3>
 *  <h3> For user </h3>
 *  @li User should provide valid buffer and buffer_size parameters. No NULL pointer allowed as weel as invalid pointer.
 */
/** @ingroup api_storage
 *  @section api_storage_length Write action
 *
 *  Used to write byte array to data from specified continuous buffer.
 *
 *  <h3> For developer </h3>
 *  <h3> For user </h3>
 *  @li User should provide valid buffer and buffer_size parameters. No NULL pointer allowed as weel as invalid pointer.
 */
typedef struct fastcall_io {
	fastcall_header        header;
	uintmax_t              offset;
	void                  *buffer;
	uintmax_t              buffer_size;
} fastcall_io;
typedef struct fastcall_io     fastcall_read;  // ACTION(READ)
typedef struct fastcall_io     fastcall_write; // ACTION(WRITE)

// }}}
// Basic arithmetic api set {{{
/** @ingroup api
 *  @addtogroup api_arith Basic arithmetic api set
 */
/** @ingroup api_arith
 *  @page api_arith_overview Overview
 *  
 *  This is api set for arithmetic computations. Probably useful only for numerics, but still should be.
 */

/** @ingroup api_arith
 *  @section api_arith_add Increment, decrement, add, sub, multiply and divide actions
 *
 *  Used to make increments, decrements, +, -, *, / with data.
 *
 *  <h3> For developer </h3>
 *  <h3> For user </h3>
 *  @li Better to provide data with same datatypes, or expect error
 */
typedef struct fastcall_arith {
	fastcall_header        header;
	data_t                *data2;
} fastcall_arith;
typedef struct fastcall_arith  fastcall_add; // ACTION(ADD)
typedef struct fastcall_arith  fastcall_sub; // ACTION(SUB)
typedef struct fastcall_arith  fastcall_mul; // ACTION(MULTIPLY)
typedef struct fastcall_arith  fastcall_div; // ACTION(DIVIDE)

typedef struct fastcall_arith_no_arg {
	fastcall_header        header;
} fastcall_arith_no_arg;
typedef struct fastcall_arith_no_arg fastcall_increment; // ACTION(INCREMENT)
typedef struct fastcall_arith_no_arg fastcall_decrement; // ACTION(DECREMENT)

// }}}
// Compare api set {{{
/** @ingroup api
 *  @addtogroup api_compare Compare api set
 */
/** @ingroup api_compare
 *  @page api_compare_overview Overview
 *  
 *  This is api set for comparisons between datatypes.
 */

/** @ingroup api_compare
 *  @section api_compare_is_null Is null action
 *
 *  Used to get is current data equals to same type empty data.
 *
 *  <h3> For developer </h3>
 *  <h3> For user </h3>
 */
typedef struct fastcall_is_null { // ACTION(IS_NULL)
	fastcall_header        header;
	uintmax_t              is_null;
} fastcall_is_null;

/** @ingroup api_compare
 *  @section api_compare_compare Compare action
 *
 *  Compare current and given data. Result written to ->result field.
 *
 *  @return 0 Data equals
 *  @return 1 Current data is less
 *  @return 2 Given data is less
 *
 *  <h3> For developer </h3>
 *  <h3> For user </h3>
 */
typedef struct fastcall_compare { // ACTION(COMPARE)
	fastcall_header        header;
	data_t                *data2;
	uintmax_t              result;
} fastcall_compare;
// }}}
// Items api set {{{
/** @ingroup api
 *  @addtogroup api_items Items api set
 */
/** @ingroup api_items
 *  @page api_items_overview Overview
 *  
 *  This is api set is for container type of datatypes. It used to work with items stored inside containers.
 */

/** @ingroup api_items
 *  @section api_items_create Create action
 *  
 *  Create new item in container.
 *
 *  <h3> For developer </h3>
 *  <h3> For user </h3>
 */
/** @ingroup api_items
 *  @section api_items_lookup Lookup action
 *  
 *  Read item from container with given key. Returned item is copy or safe reference to stored item.
 *
 *  <h3> For developer </h3>
 *  @li Return ref_t or copy of data
 *  <h3> For user </h3>
 *  @li Free data then it is no longer need.
 */
/** @ingroup api_items
 *  @section api_items_update Update action
 *  
 *  Update item in container with given key. Value can be consumed.
 *
 *  <h3> For developer </h3>
 *  <h3> For user </h3>
 *  @li Get ready to data consuming process.
 *  @li Free value then it is no longer need.
 */
/** @ingroup api_items
 *  @section api_items_delete Delete action
 *  
 *  Delete item in container with given key.
 *
 *  @param value If specified - return deleted item (same as pop but for given key)
 *
 *  <h3> For developer </h3>
 *  <h3> For user </h3>
 */
typedef struct fastcall_crud {
	fastcall_header        header;
	data_t                *key;
	data_t                *value;
} fastcall_crud;
typedef struct fastcall_crud fastcall_create; // ACTION(CREATE)
typedef struct fastcall_crud fastcall_lookup; // ACTION(LOOKUP)
typedef struct fastcall_crud fastcall_update; // ACTION(UPDATE)
typedef struct fastcall_crud fastcall_delete; // ACTION(DELETE)

/** @ingroup api_items
 *  @section api_items_push Push action
 *  
 *  Push item in container. Pushed item can be consumed.
 *
 *  <h3> For developer </h3>
 *  <h3> For user </h3>
 *  @li Get ready to data consuming process.
 *  @li Free value then it is no longer need.
 */
typedef struct fastcall_push { // ACTION(PUSH)
	fastcall_header        header;
	data_t                *data;
} fastcall_push;

/** @ingroup api_items
 *  @section api_items_pop Pop action
 *  
 *  Pop item from container.
 *
 *  <h3> For developer </h3>
 *  <h3> For user </h3>
 *  @li Free data then it is no longer need
 */
typedef struct fastcall_pop { // ACTION(POP)
	fastcall_header        header;
	data_t                *data;
} fastcall_pop;

/** @ingroup api_items
 *  @section api_items_enum Enum action
 *  
 *  Enumerate all items in container. 
 *
 *  <h3> For developer </h3>
 *  @li Return ref_t or copy of data
 *  <h3> For user </h3>
 *  @li Free data then it is no longer need
 */
typedef struct fastcall_enum { // ACTION(ENUM)
	fastcall_header        header;
	data_t                *dest;
} fastcall_enum;
// }}}
// Data views api set {{{
/** @ingroup api
 *  @addtogroup api_view Data view api set
 */
/** @ingroup api_view
 *  @page api_view_overview Overview
 *  
 *  This is view api set. It used for creation of current view of data value.
 */

/** @ingroup api_view
 *  @section api_view_view View action
 *
 *  Used to create view for current data. It return pointer <ptr> to continious in memory chunk with length <length>.
 *  
 *  @param ptr      Pointer to memory chunk
 *  @param length   Length of memory chunk
 *  @param freeit   Data containing this view.
 *
 *  <h3> For developer </h3>
 *  <h3> For user </h3>
 *  @li Caller should free <freeit> data after call
 */
typedef struct fastcall_view { // ACTION(VIEW)
	fastcall_header        header;
	uintmax_t              format;
	void                  *ptr;
	uintmax_t              length;
	data_t                 freeit;
} fastcall_view;
// }}}

typedef struct fastcall_query {
	fastcall_header        header;
	request_t             *request;
} fastcall_query;

typedef struct fastcall_batch { // ACTION(BATCH)
	fastcall_header        header;
	fastcall_header      **requests;
} fastcall_batch;


extern uintmax_t fastcall_nargs[];

ssize_t     data_hash_query(data_t *data, request_t *request);
ssize_t     data_list_query(data_t *data, request_t *list);
ssize_t     machine_fast_query(machine_t *machine, void *hargs);

ssize_t     api_machine_nosys  (machine_t *machine, request_t *request);
ssize_t     api_data_nosys     (data_t *data, fastcall_header *hargs);
#endif

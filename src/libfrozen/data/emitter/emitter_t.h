#ifndef DATA_EMITTER_T
#define DATA_EMITTER_T

/** @ingroup data
 *  @addtogroup emitter_t emitter_t
 */
/** @ingroup emitter_t
 *  @page emitter_t_overview Overview
 *  
 *  This data type emits request to supplied backend when receive any data action.
 *
 */
/** @ingroup emitter_t
 *  @page emitter_t_define Define
 *
 *  Possible defines:
 *  @code
 *       data_t emitter = DATA_EMITTERT(backend, request); // emit to backend with request
 *  @endcode
 */

#define DATA_EMITTERT(_backend,_request)  {TYPE_EMITTERT, (emitter_t []){ { _backend, _request, 0 } }}
#define DEREF_TYPE_EMITTERT(_data) (emitter_t *)((_data)->ptr)
#define REF_TYPE_EMITTERT(_dt) _dt
#define HAVEBUFF_TYPE_EMITTERT 0

typedef struct emitter_t {
	backend_t             *backend;
	request_t             *request;
	
	// internal
	uintmax_t              allocated;
} emitter_t;

#endif

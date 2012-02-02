#ifndef DATA_CONTAINER_T_H
#define DATA_CONTAINER_T_H

/** @ingroup data
 *  @addtogroup container_t container_t
 */
/** @ingroup container_t
 *  @page container_t_overview Overview
 *  
 *  This data used to hold any data within itself and make calls to whole set of data as it was glued together.
 */
/** @ingroup container_t
 *  @page container_t_define Define
 *
 *  Possible defines:
 *  @code
 *       data_t hcontainer = DATA_CONTAINERT(container);
 *  @endcode
 */

#define DATA_CONTAINERT()           { TYPE_CONTAINERT, (container_t []){{ NULL, NULL }} }
#define DATA_HEAP_CONTAINERT()      { TYPE_CONTAINERT, container_alloc()                }
#define DATA_PTR_CONTAINERT(_buff)  { TYPE_CONTAINERT, (void *)_buff                    }
#define DEREF_TYPE_CONTAINERT(_data) (container_t *)((_data)->ptr)
#define REF_TYPE_CONTAINERT(_dt) _dt
#define HAVEBUFF_TYPE_CONTAINERT 0

/** @file container.h */

typedef struct container_t container_t;
typedef struct container_chunk_t container_chunk_t;

typedef enum chunk_flags_t {
	CHUNK_CACHE_SIZE = 1,
	CHUNK_ADOPT_DATA = 2,
	CHUNK_DONT_FREE  = 4,

	INTERNAL_CACHED  = 16
} chunk_flags_t;

/// chunk_t structure
struct container_chunk_t {
	container_chunk_t     *cnext;         ///< Next chunk ptr
	data_t                 data;          ///< Data holder
	uintmax_t              cached_size;   ///< Chunk size
	chunk_flags_t          flags;         ///< Chunk flags
};

/// container_t structure
struct container_t {
	container_chunk_t     *head;          ///< First container chunk
	container_chunk_t     *tail;          ///< Last container chunk
};

API void            container_init                (container_t *container);
API void            container_destroy             (container_t *container);

API container_t *   container_alloc               (void);
API void            container_free                (container_t *container);

API ssize_t         container_add_head_data       (container_t *container, data_t *data, chunk_flags_t flags);
API ssize_t         container_add_tail_data       (container_t *container, data_t *data, chunk_flags_t flags);

API void            container_clean               (container_t *container);
API uintmax_t       container_size                (container_t *container);

API ssize_t         container_write               (container_t *container, uintmax_t offset, void *buf, uintmax_t buf_size, uintmax_t *written);
API ssize_t         container_read                (container_t *container, uintmax_t offset, void *buf, uintmax_t buf_size, uintmax_t *read);

#endif

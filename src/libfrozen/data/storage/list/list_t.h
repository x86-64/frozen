#ifndef DATA_LIST_T_H
#define DATA_LIST_T_H

/** @ingroup data
 *  @addtogroup list_t list_t
 */
/** @ingroup list_t
 *  @page list_t_overview Overview
 *  
 *  This data used to hold any data within itself.
 */
/** @ingroup list_t
 *  @page list_t_define Define
 *
 *  Possible defines:
 *  @code
 *       data_t hlist = DATA_LISTT(list);
 *  @endcode
 */

#define DATA_LISTT()      { TYPE_LISTT, (list_t []){ { NULL } }
#define DATA_PTR_LISTT(_buff)   { TYPE_LISTT, (void *)_buff }
#define DEREF_TYPE_LISTT(_data) (list_t *)((_data)->ptr)
#define REF_TYPE_LISTT(_dt) _dt
#define HAVEBUFF_TYPE_LISTT 0


/** @file list.h */

typedef struct list_t       list_t;
typedef struct list_chunk_t list_chunk_t;

struct list_chunk_t {
	list_chunk_t          *cnext;         ///< Next chunk ptr
	data_t                 data;          ///< Data holder
};

/// list_t structure
struct list_t {
	list_chunk_t     *head;               ///< First list chunk
};

API void            list_t_init                (list_t *list);
API void            list_t_destroy             (list_t *list);

API list_t *        list_t_alloc               (void);
API void            list_t_free                (list_t *list);

API ssize_t         list_t_push                (list_t *list, data_t *data);
API ssize_t         list_t_pop                 (list_t *list, data_t *data);

#endif

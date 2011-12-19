#ifndef DATA_STRUCT_H
#define DATA_STRUCT_H

/** @file struct_t.h */
/** @ingroup data
 *  @addtogroup struct_t struct_t
 */
/** @ingroup struct_t
 *  @page struct_t_overview Overview
 *  
 *  This data type format input data according defined structure.
 */
/** @ingroup struct_t
 *  @page struct_t_define Define
 *
 *  Possible defines:
 *  @code
 *       data_t hstruct = DATA_STRUCTT(structure, values);
 *  @endcode
 */

#define DATA_STRUCTT(_struct,_values) {TYPE_STRUCTT, (struct_t []){ { _struct, _values } }}
#define DEREF_TYPE_STRUCTT(_data) (struct_t *)((_data)->ptr)
#define REF_TYPE_STRUCTT(_dt) _dt
#define HAVEBUFF_TYPE_STRUCTT 0

typedef struct struct_t {
	hash_t                *structure;
	hash_t                *values;
} struct_t;

uintmax_t    struct_pack         (hash_t *structure, request_t *values, data_t *buffer);
uintmax_t    struct_unpack       (hash_t *structure, request_t *values, data_t *buffer);

#endif

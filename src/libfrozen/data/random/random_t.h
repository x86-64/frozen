#ifndef DATA_RANDOM_T
#define DATA_RANDOM_T

/** @ingroup data
 *  @addtogroup random_t random_t
 */
/** @ingroup random_t
 *  @page random_t_overview Overview
 *  
 *  This data output random data on reads. It use stdlib routine random(), so do not rely on security here.
 */
/** @ingroup random_t
 *  @page random_t_define Define
 *
 *  Possible defines:
 *  @code
 *       data_t random = DATA_RANDOMT();
 *  @endcode
 */

#define DATA_RANDOMT  { TYPE_RANDOMT, NULL }
#define DEREF_TYPE_RANDOMT(_data) ((_data)->ptr)
#define REF_TYPE_RANDOMT(_dt) _dt
#define HAVEBUFF_TYPE_RANDOMT 0

#endif

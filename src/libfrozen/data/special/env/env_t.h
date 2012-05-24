#ifndef DATA_ENV_T
#define DATA_ENV_T

/** @ingroup data
 *  @addtogroup env_t env_t
 */
/** @ingroup env_t
 *  @page env_t_overview Overview
 *  
 *  This data is linked to current request key, if exist such.
 */
/** @ingroup env_t
 *  @page env_t_define Define
 *
 *  Possible defines:
 *  @code
 *       data_t env = DATA_ENVT(HK(key));
 *  @endcode
 */

#define DATA_ENVT(_key)  { TYPE_ENVT, (env_t []){ { _key } }}
#define DEREF_TYPE_ENVT(_data) (env_t *)((_data)->ptr)
#define REF_TYPE_ENVT(_dt) _dt
#define HAVEBUFF_TYPE_ENVT 0

typedef struct env_t {
	hashkey_t              key;
} env_t;

API ssize_t data_env_t(data_t *data, hashkey_t key);

#endif

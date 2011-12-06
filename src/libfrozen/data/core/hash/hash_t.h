#ifndef DATA_HASH_T_H
#define DATA_HASH_T_H

/** @ingroup hash_t
 *  @page hash_t_define Define
 *
 *  Possible defines:
 *  @code
 *      hash_t  newhash1[] = {                              // actual hash
 *          { HK(key),   DATA_UINTT(100) },
 *          hash_end
 *      };
 *      data_t  hnewhash1  = DATA_PTR_HASHT(&newhash1);     // data holder
 *      
 *      data_t  hnewhash2  = DATA_HASHT(                    // only data holder
 *           { HK(key),  DATA_UINTT(200) },
 *           hash_end
 *      );
 *
 *      hash_t  somehash[] = {                              // as key in another hash
 *           { HK(hash), DATA_HASHT(
 *                  { HK(key),  DATA_UINTT(300) },
 *                  hash_end
 *           )},
 *           hash_end
 *      };
 *  @endcode
 */

#define DATA_HASHT(...)             { TYPE_HASHT, (hash_t []){ __VA_ARGS__ } }
#define DATA_PTR_HASHT(_hash)       { TYPE_HASHT, (void *)_hash }
#define DEREF_TYPE_HASHT(_data) (hash_t *)((_data)->ptr)
#define REF_TYPE_HASHT(_dt) _dt
#define HAVEBUFF_TYPE_HASHT 0

#endif

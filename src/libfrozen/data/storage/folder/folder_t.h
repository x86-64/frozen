#ifndef DATA_FOLDER_T_H
#define DATA_FOLDER_T_H

/** @ingroup data
 *  @addtogroup folder_t folder_t
 */
/** @ingroup folder_t
 *  @page folder_t_overview Overview
 *  
 * This data can enum folder content
 */
/** @ingroup folder_t
 *  @page folder_t_define Configuration
 * 
 * Accepted configuration:
 * @code
 * some = (folder_t){
 *              path        = "somefoldername/"       # simple folder path
 * }
 * @endcode
 */

//#define DATA_FOLDERT(...)      { TYPE_FOLDERT, (folder_t []){ __VA_ARGS__ } }
//#define DATA_PTR_FOLDERT(...)  { TYPE_FOLDERT, __VA_ARGS__ }
//#define DEREF_TYPE_FOLDERT(_data) *(folder_t *)((_data)->ptr)
//#define REF_TYPE_FOLDERT(_dt) (&(_dt))
//#define HAVEBUFF_TYPE_FOLDERT 1

#endif

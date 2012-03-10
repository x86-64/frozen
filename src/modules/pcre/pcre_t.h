#ifndef DATA_PCRET_H
#define DATA_PCRET_H

/** @ingroup data
 *  @addtogroup pcre_t pcre_t
 */
/** @ingroup pcre_t
 *  @page pcre_t_overview Overview
 *  
 *  This data used to match pcre
 */

datatype_t            type_pcret;

#define PCRET_NAME  "pcre_t"
#define TYPE_PCRET  type_pcret 

#define DATA_PCRET(value) { TYPE_PCRET, (pcre_t []){ value } } 
#define DATA_PTR_PCRET(value) { TYPE_PCRET, value } 
#define DEREF_TYPE_PCRET(_data) (pcre_t *)((_data)->ptr) 
#define REF_TYPE_PCRET(_dt) _dt 
#define HAVEBUFF_TYPE_PCRET 0

#endif

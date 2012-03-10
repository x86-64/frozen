#ifndef DATA_REGEXPT_H
#define DATA_REGEXPT_H

/** @ingroup data
 *  @addtogroup regexp_t regexp_t
 */
/** @ingroup regexp_t
 *  @page regexp_t_overview Overview
 *  
 *  This data used to match regexp
 */

datatype_t            type_regexpt;

#define REGEXPT_NAME  "regexp_t"
#define TYPE_REGEXPT  type_regexpt 

#define DATA_REGEXPT(value) { TYPE_REGEXPT, (regexp_t []){ value } } 
#define DATA_PTR_REGEXPT(value) { TYPE_REGEXPT, value } 
#define DEREF_TYPE_REGEXPT(_data) (regexp_t *)((_data)->ptr) 
#define REF_TYPE_REGEXPT(_dt) _dt 
#define HAVEBUFF_TYPE_REGEXPT 0

#endif

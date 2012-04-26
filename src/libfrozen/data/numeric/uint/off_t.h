#line 1 "uint_init.m4"






















#line 1 "uint.h.m4"


#ifndef DATA_UINT_OFFT_H
#define DATA_UINT_OFFT_H

/** @ingroup data
 *  @addtogroup off_t off_t
 */
/** @ingroup off_t
 *  @page off_t_overview Overview
 *  
 *  This data used to hold integers of different size.
 */
/** @ingroup off_t
 *  @page off_t_define Define
 *
 *  Possible defines:
 *  @code
 *       data_t hint  = DATA_OFFT(100);
 *        
 *       off_t some  = 200;
 *       data_t hpint = DATA_PTR_OFFT(&some);
 *  @endcode
 */

#define DATA_OFFT(value) { TYPE_OFFT, (off_t []){ value } } 
#define DATA_HEAP_OFFT(value) { TYPE_OFFT, data_off_t_alloc(value) } 
#define DATA_PTR_OFFT(value) { TYPE_OFFT, value } 
#define DEREF_TYPE_OFFT(_data) *(off_t *)((_data)->ptr) 
#define REF_TYPE_OFFT(_dt) (&(_dt)) 
#define HAVEBUFF_TYPE_OFFT 1

off_t * data_off_t_alloc(off_t value);

#endif
/* vim: set filetype=m4: */

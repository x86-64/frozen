#ifndef DATA_IPV4T_H
#define DATA_IPV4T_H

/** @ingroup data
 *  @addtogroup ipv4_t ipv4_t
 */
/** @ingroup ipv4_t
 *  @page ipv4_t_overview Overview
 *  
 *  This data used to hold ipv4 address.
 */

#define IPV4T_NAME  "ipv4_t"
#define TYPE_IPV4T  datatype_t_getid_byname(IPV4T_NAME, NULL)

#define DATA_IPV4T(value) { TYPE_IPV4T, (ipv4_t []){ value } } 
#define DATA_PTR_IPV4T(value) { TYPE_IPV4T, value } 
#define DEREF_TYPE_IPV4T(_data) (ipv4_t *)((_data)->ptr) 
#define REF_TYPE_IPV4T(_dt) _dt 
#define HAVEBUFF_TYPE_IPV4T 0

typedef struct ipv4_t {
	uint32_t               ip;
} ipv4_t;

#endif

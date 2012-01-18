#ifndef DATA_ACTION_T_H
#define DATA_ACTION_T_H

#include <enum/action/action.h>

/** @ingroup data
 *  @addtogroup action_t action_t
 */
/** @ingroup action_t
 *  @page action_t_overview Overview
 *  
 *  This data used to hold actions
 */

#define DATA_ACTIONT(...)  { TYPE_ACTIONT, (action_t []){ __VA_ARGS__ } }
#define DATA_PTR_ACTIONT(...)  { TYPE_ACTIONT, __VA_ARGS__ }
#define DEREF_TYPE_ACTIONT(_data) *(action_t *)((_data)->ptr)
#define REF_TYPE_ACTIONT(_dt) (&(_dt))
#define HAVEBUFF_TYPE_ACTIONT 1

#endif

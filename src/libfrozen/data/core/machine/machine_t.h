#ifndef DATA_MACHINE_H
#define DATA_MACHINE_H

/** @ingroup data
 *  @addtogroup machine_t machine_t
 */
/** @ingroup machine_t
 *  @page machine_t_overview Overview
 *  
 *  This data used to hold machines as data.
 *
 *  To create machine_t use conversion from hash (as config parameters), from string (as machine name) or machine api.
 */
/** @ingroup machine_t
 *  @page machine_t_define Define
 *
 *  Possible defines:
 *  @code
 *       data_t htype = DATA_MACHINET(machine);            // current machine
 *       data_t htype = DATA_NEXTMACHINET(machine);        // next machine in shop
 *  @endcode
 */

#define DATA_MACHINET(_machine)      { TYPE_MACHINET, _machine        }
#define DATA_NEXT_MACHINET(_machine) { TYPE_MACHINET, _machine->cnext }
#define DEREF_TYPE_MACHINET(_data) (machine_t *)((_data)->ptr)
#define REF_TYPE_MACHINET(_dt) _dt
#define HAVEBUFF_TYPE_MACHINET 1

#endif

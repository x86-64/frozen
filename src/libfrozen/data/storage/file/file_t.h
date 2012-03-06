#ifndef DATA_FILE_T_H
#define DATA_FILE_T_H

/** @ingroup data
 *  @addtogroup file_t file_t
 */
/** @ingroup file_t
 *  @page file_t_overview Overview
 *  
 * This data can read and write to file
 */
/** @ingroup file_t
 *  @page file_t_define Configuration
 * 
 * Accepted configuration:
 * @code
 * some = (file_t){
 *              filename    = "somefilename.dat",     // simple file path
 *              readonly    = (uint_t)'1',            // make file read-only, default is read-write
 *              exclusive   = (uint_t)'1',            // pass O_EXCL for open, see man page, default 0
 *              create      = (uint_t)'1',            // pass O_CREAT for open, see man page, default 1
 *              mode        = (uint_t)'0777',         // file mode on creation, default 600
 *              retry       = (uint_t)'1',            // regen filename and retry open. useful for example for tmpname generation, default 0
 *              append      = (uint_t)'1',            // append mode, default 0
 *              truncate    = (uint_t)'1'             // truncate file on open, default 0
 * }
 *
 * some = (file_t)"FILENAME"                          // open file with default parameters
 * @endcode
 */

//#define DATA_FILET(...)      { TYPE_FILET, (file_t []){ __VA_ARGS__ } }
//#define DATA_PTR_FILET(...)  { TYPE_FILET, __VA_ARGS__ }
//#define DEREF_TYPE_FILET(_data) *(file_t *)((_data)->ptr)
//#define REF_TYPE_FILET(_dt) (&(_dt))
//#define HAVEBUFF_TYPE_FILET 1

#endif

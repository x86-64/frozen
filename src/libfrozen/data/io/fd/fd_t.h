#ifndef DATA_FD_T
#define DATA_FD_T

/** @file fd_t.h */
/** @ingroup data
 *  @addtogroup fd_t fd_t
 */
/** @ingroup fd_t
 *  @page fd_t_overview Overview
 *  
 *  This data used to work with file descriptors, created by socket() or open()
 */
/** @ingroup fd_t
 *  @page fd_t_define Define
 *
 *  Possible defines:
 *  @code
 *       data_t hfd = DATA_FDT(filedescriptor);
 *  @endcode
 */

#define DATA_FDT(_fd, _seekable)  { TYPE_FDT, (fd_t []){ { _fd, _seekable } } }
#define DEREF_TYPE_FDT(_data) (fd_t *)((_data)->ptr)
#define REF_TYPE_FDT(_dt) _dt
#define HAVEBUFF_TYPE_FDT 0

typedef struct fd_t {
	int                    fd;
	int                    seekable;
} fd_t;

#endif

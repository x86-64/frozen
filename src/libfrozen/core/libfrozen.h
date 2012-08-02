#ifndef LIBFROZEN_H
#define LIBFROZEN_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define _FILE_OFFSET_BITS 64

#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif

#ifndef _WITH_DPRINTF
#define _WITH_DPRINTF // for snprintf on FreeBSD
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

/* Standart libraries */

#ifdef STDC_HEADERS
# include <stdlib.h>
# include <stddef.h>
#else
# ifdef HAVE_STDLIB_H
#  include <stdlib.h>
# endif
#endif
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/param.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <inttypes.h>

#ifndef alloca

#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#elif defined __GNUC__
# define alloca __builtin_alloca
#elif defined _AIX
# define alloca __alloca
#elif defined _MSC_VER
# include <malloc.h>
# define alloca _alloca
#else
# ifndef HAVE_ALLOCA
#  ifdef  __cplusplus
extern "C"
#  endif
void *alloca (size_t);
# endif
#endif

#endif

#define _inline   extern inline
#define API       __attribute__((visibility("default")))

#define __HALF_MAX_SIGNED(type) ((type)1 << (sizeof(type)*8-2))
#define __MAX_SIGNED(type) (__HALF_MAX_SIGNED(type) - 1 + __HALF_MAX_SIGNED(type))
#define __MIN_SIGNED(type) (-1 - __MAX_SIGNED(type))
#define __MIN(type) ((type)-1 < 1?__MIN_SIGNED(type):(type)0)
#define __MAX(type) ((type)~__MIN(type))

/* Own headers */
#include <list.h>
#include <errors.h>
#include <public.h>
#include <paths.h>

#include <configs/config.h>
#include <enums.h>

#include <data.h>
#include <api.h>
#include <data_selected.h>
#include <machine.h>
#include <machine_selected.h>
#include <helpers.h>

extern char  *frozen_modules_dir;

typedef void (*f_signal_handler)(int sig, void *userdata);


API int       frozen_init(void);
API int       frozen_destroy(void);

API void      frozen_set_modules_dir(char *dir);
API ssize_t   frozen_subscribe_signal(int sig, f_signal_handler handler, void *userdata);
API void      frozen_unsubscribe_signal(int sig, f_signal_handler handler, void *userdata);

    intmax_t  safe_pow(uintmax_t *res, uintmax_t x, uintmax_t y);
    intmax_t  safe_mul(uintmax_t *res, uintmax_t x, uintmax_t y);
    intmax_t  safe_div(uintmax_t *res, uintmax_t x, uintmax_t y);
API void *    memdup(void *ptr, uintmax_t size);
API uintmax_t portable_hash(char *p);

#endif

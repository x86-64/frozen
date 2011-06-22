#ifndef LIBFROZEN_H
#define LIBFROZEN_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define _FILE_OFFSET_BITS 64

#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif

#ifndef __USE_GNU
#define __USE_GNU
#endif

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 500 
#endif

#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif

/* Standart libraries */
#include <stdint.h>
#include <stdlib.h>
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
#include <alloca.h>

#define _inline   extern inline
#define API       __attribute__((visibility("default")))

#define __HALF_MAX_SIGNED(type) ((type)1 << (sizeof(type)*8-2))
#define __MAX_SIGNED(type) (__HALF_MAX_SIGNED(type) - 1 + __HALF_MAX_SIGNED(type))
#define __MIN_SIGNED(type) (-1 - __MAX_SIGNED(type))
#define __MIN(type) ((type)-1 < 1?__MIN_SIGNED(type):(type)0)
#define __MAX(type) ((type)~__MIN(type))

/* Own headers */
#include <errors.h>
#include <public.h>

#include <list.h>               // internal data storage
#include <buffer.h>             //
#include <data_selected_enum.h> //
#include <data.h>               //
#include <hash.h>               //

#ifndef DATA_TYPE_SOURCE
	#include <data_selected.h>
#endif


#include <configs/config.h>  // confiurations
#include <backend.h>         // db logic 

/* Global variables */
API extern hash_t *global_settings;

API int       frozen_init(void);
API int       frozen_destroy(void);
    intmax_t  safe_pow(uintmax_t *res, uintmax_t x, uintmax_t y);
    intmax_t  safe_mul(uintmax_t *res, uintmax_t x, uintmax_t y);
    intmax_t  safe_div(uintmax_t *res, uintmax_t x, uintmax_t y);

#endif

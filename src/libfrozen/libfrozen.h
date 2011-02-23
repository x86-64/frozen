#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define _FILE_OFFSET_BITS 64
#define _LARGEFILE64_SOURCE
#define __USE_GNU
#define _XOPEN_SOURCE 500 

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
#include <fcntl.h>
#include <unistd.h>
#include <alloca.h>

#define _inline   extern inline
#define API       __attribute__((visibility("default")))

#define error(...)            do{ fprintf(stdout,__VA_ARGS__);    }while(0);
#define return_error(err,...) do{ error(__VA_ARGS__); return err; }while(0);
#define label_error(err,...)  do{ error(__VA_ARGS__); goto err;   }while(0);

#define MIN(a,b)              ( (a > b) ? b : a )
#define MAX(a,b)              ( (a > b) ? a : b )

#define __HALF_MAX_SIGNED(type) ((type)1 << (sizeof(type)*8-2))
#define __MAX_SIGNED(type) (__HALF_MAX_SIGNED(type) - 1 + __HALF_MAX_SIGNED(type))
#define __MIN_SIGNED(type) (-1 - __MAX_SIGNED(type))
#define __MIN(type) ((type)-1 < 1?__MIN_SIGNED(type):(type)0)
#define __MAX(type) ((type)~__MIN(type))

/* Own headers */
#include <public.h>

#include <list.h>            // internal data storage
#include <buffer.h>          //
#include <data.h>            //
#include <hash.h>            //

#include <configs/config.h>  // confiurations
#include <backend.h>         // db logic 

/* Global variables */
API extern hash_t *global_settings;

API int frozen_init(void);
API int frozen_destroy(void);


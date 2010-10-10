#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define DEBUG
#define _inline extern inline

#define _FILE_OFFSET_BITS 64
#define _LARGEFILE64_SOURCE
#define __USE_GNU
#define _XOPEN_SOURCE 500 

/* Standart libraries */
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define error(...)            do{ fprintf(stdout,__VA_ARGS__);    }while(0);
#define return_error(err,...) do{ error(__VA_ARGS__); return err; }while(0);
#define MIN(a,b)              ( (a > b) ? b : a )


/* Own headers */
#include <list.h>            // internal data storage
#include <settings.h>        // 
#include <buffer.h>          //
#include <data.h>            //
#include <structs.h>         //
#include <hash.h>            //

#include <public.h>

#include <backend.h>         // db logic 

/* Global variables */
extern setting_t *global_settings;

int frozen_init(void);
int frozen_destory(void);


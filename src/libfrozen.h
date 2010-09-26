#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define DEBUG
#define _inline  extern inline

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
#include <data_protos.h>     //
#include <data.h>            //
#include <buffer.h>          //
#include <structs.h>         //
#include <hash.h>            //

#include <public.h>

#include <backend.h>         // db logic 

/* Global variables */
extern setting_t *global_settings;


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define _inline  extern inline

/* Standart libraries */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


/* Own headers */
#include <list.h>
#include <settings.h>
#include <data_types.h>
#include <data.h>
#include <structs.h>
#include <buffer.h>
#include <structs_buffer.h>
#include <backend.h>

/* Global variables */
extern setting_t *global_settings;



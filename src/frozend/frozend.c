#include <libfrozen.h>
#include <paths.h>

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <signal.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <dirent.h>
#include <strings.h>

/* global options */
char            defopt_config_file[] = "frozen.conf";
char            defopt_modules_dir[] = FROZEN_MODULESDIR;

char           *opt_pidfile        = NULL;
char           *opt_config_file    = defopt_config_file;
char           *opt_modules_dir    = defopt_modules_dir;
unsigned int    opt_daemon         = 0;

/* getopt arrays  {{{ */
struct cmdline_option {
	const char *long_name;
	char        short_name;
	enum {
		OPT_VALUE_INT,
		OPT_VALUE_STR,
		OPT_VALUE_BOOL,
		OPT_FUNC,
		OPT_GROUP
	} type; 
	
	void       *opt_ptr;
	const char *helpmsg;
}; 
// }}}

typedef void (*opt_func)(void);
void opts_help(void);
void main_rest(void);
void main_cleanup(void);

static struct cmdline_option option_data[] = { 
	{ "help",           'h', OPT_FUNC,       (void *)&opts_help,    "help screen"              },
	{ "config",         'c', OPT_VALUE_STR,  &opt_config_file,      "file with configuration"  },
	{ "daemon",         'd', OPT_VALUE_BOOL, &opt_daemon,           "daemonize"                },
	{ "pid-file",        0,  OPT_VALUE_STR,  &opt_pidfile,          "save pid to pidfile"      },
	{ "modules",         0,  OPT_VALUE_STR,  &opt_modules_dir,      "frozen modules dir"       },
	{ "m4-path",        'p', OPT_VALUE_STR,  &config_m4_path,       "m4 path"                  },
	{ "m4-opts",        'o', OPT_VALUE_STR,  &config_m4_opts,       "m4 options"               },

	{ NULL, 0, 0, NULL, NULL }
};

static struct option            long_options  [sizeof(option_data)/sizeof(struct cmdline_option) + 1]; 
static char                     short_options [sizeof(option_data)/sizeof(struct cmdline_option) + 1]; 
static struct cmdline_option *  map_opts      [96]; 

// daemonization {{{
void daemonize(void){ // {{{
	int i;
	i=fork();
	if (i<0) exit(1); /* fork error */
	if (i>0) exit(0); /* parent exits */
	/* child (daemon) continues */
	setsid(); /* obtain a new process group */
	for (i=65535;i>=0;--i) close(i); /* close all descriptors */
	i=open("/dev/null",O_RDWR); dup(i); dup(i); /* handle standart I/O */
	signal(SIGCHLD,SIG_IGN); /* ignore child */
	signal(SIGTSTP,SIG_IGN); /* ignore tty signals */
	signal(SIGTTOU,SIG_IGN);
	signal(SIGTTIN,SIG_IGN);
} // }}}
// }}}
// pid functions from memcachedb with modifications {{{
static void save_pid(const char *pid_file) { // {{{
    FILE *fp; 
    pid_t pid = getpid();

    if (pid_file == NULL)
        return;

    if ((fp = fopen(pid_file, "w")) == NULL) {
        fprintf(stderr, "Could not open the pid file %s for writing\n", pid_file);
        return;
    }    

    fprintf(fp,"%ld\n", (long)pid);
    if (fclose(fp) == -1) {
        fprintf(stderr, "Could not close the pid file %s.\n", pid_file);
        return;
    }    
} // }}}
static void remove_pidfile(const char *pid_file) { // {{{
  if (pid_file == NULL)
      return;

  if (unlink(pid_file) != 0) { 
      fprintf(stderr, "Could not remove the pid file %s.\n", pid_file);
  }

} // }}}
// }}}
// options {{{
void opts_init(void){ // {{{
	int                    i;
	struct cmdline_option *mopt = option_data;
	struct option         *lopt = long_options;
	char                  *sopt = short_options;
	
	i = 0;
	while(mopt->long_name != NULL){
		if(mopt->type == OPT_GROUP)
			goto next;
		
		lopt->name       = mopt->long_name;
		lopt->val        = i;
		
		if(mopt->short_name){
			*sopt++                         = mopt->short_name;
			map_opts[mopt->short_name - 32] = mopt;
		}
		
		switch(mopt->type){
			case OPT_VALUE_STR:
			case OPT_VALUE_INT:
				lopt->has_arg = required_argument;
				if(mopt->short_name)
					*sopt++       = ':';
				break;
			case OPT_VALUE_BOOL:
			case OPT_FUNC:
				lopt->has_arg = no_argument;
				break;
			case OPT_GROUP:
				break;
		}
		
		lopt++;
	next:
		mopt++;
		i++;
	}
	*sopt = '\0';
} // }}}
void opts_help(void){ // {{{
	fprintf(stderr, "usage: frozend [OPTIONS]\n\nOptions:\n");
	struct cmdline_option *mopt = option_data;
	char                  *p;
	char                   col1[25]; 
	
	while(mopt->long_name != NULL){
		if(mopt->type == OPT_GROUP){
			fprintf(stderr, "%s\n", mopt->long_name);
			goto next;
		}

		memset(col1 + 0,  ' ', 25);
		p = col1 + 2;
		memset(p,  '-', 2);                                  p += 2;
		memcpy(p, mopt->long_name, strlen(mopt->long_name)); p += strlen(mopt->long_name);
		if(mopt->short_name){
			memset(p++, ',', 1);
			memset(p++, ' ', 1);
			memset(p++, '-', 1);
			memset(p++, mopt->short_name, 1);
		}
		
		fprintf(stderr, "%.*s%s\n", 25, col1, mopt->helpmsg);
	next:
		mopt++;
	}
	
	exit(255);
} // }}}
int  main (int argc, char **argv){ // {{{
	int c;
	int option_index;
	opt_func func;
	
	opts_init();
	
	option_index = -1;
	while( (c = getopt_long (argc, argv, short_options, long_options, &option_index)) != -1){
		struct cmdline_option *mopt;
		
		if(option_index == -1){
			mopt = map_opts[c - 32];
		}else{
			mopt = &option_data[ long_options[option_index].val ];
		}
		if(mopt == NULL)
			exit(255);
		
		switch(mopt->type){
			case OPT_VALUE_INT:
				*(unsigned int *)(mopt->opt_ptr) = atoi(optarg);
				break;
			case OPT_VALUE_STR:
				*(char **)       (mopt->opt_ptr) = strdup(optarg);
				break;
			case OPT_VALUE_BOOL:
				*(unsigned int *)(mopt->opt_ptr) = 1;
				break;
			case OPT_GROUP:
				break;
			case OPT_FUNC:
				func = (opt_func)(mopt->opt_ptr);
				func();
				exit(0);
		}
		
		option_index = -1;
	}
	
	main_rest();
	main_cleanup();	
	
	return 0;
} // }}}
// }}}
// modules {{{
//   go modules {{{
int go_inited = 0;

void module_init_go(void){
	void *libgo_ptr;
	void (*libgo_mallocinit)(void);
	void (*libgo_goroutineinit)(void *);
	void (*libgo_gc)(void);
	void (*libgo_sig)(void);
	char t[20];

	if(go_inited == 0){
		if( !(libgo_ptr = dlopen("libgo.so",            RTLD_NOW | RTLD_GLOBAL )) )
			return;
		
		// TODO error handling
		*(void **)(&libgo_mallocinit)    = dlsym(libgo_ptr, "runtime_mallocinit");
		*(void **)(&libgo_goroutineinit) = dlsym(libgo_ptr, "__go_gc_goroutine_init");
		*(void **)(&libgo_sig)           = dlsym(libgo_ptr, "__initsig");
		*(void **)(&libgo_gc)            = dlsym(libgo_ptr, "__go_enable_gc");
		
		(*libgo_mallocinit)();
		(*libgo_goroutineinit)(&t);
		(*libgo_sig)();
		//(*libgo_gc)();
		
		go_inited = 1;
	}
}

void module_load_go(void *module_handle){ // {{{
	void (*goinitmain)(void);
	void (*gomain)(void);
	
	*(void **)(&goinitmain)          = dlsym(module_handle, "__go_init_main");
	*(void **)(&gomain)              = dlsym(module_handle, "main.main");
	(*goinitmain)();
	(*gomain)();

	return;
} // }}}
int module_is_gomodule(void *module_handle){ // {{{
	return (dlsym(module_handle, "main.main") != NULL);
} // }}}
// }}}
// c modules {{{
void module_load_c(void *module_handle){ // {{{
	void (*cmain)(void);
	
	*(void **)(&cmain)              = dlsym(module_handle, "main");
	(*cmain)();

	return;
} // }}}
int module_is_cmodule(void *module_handle){ // {{{
	return (dlsym(module_handle, "main") != NULL);
} // }}}
// }}}
void modules_load(void){ // {{{
	char                  *ext;
	DIR                   *modules_dir;
	struct dirent         *dir;
	char                   module_path[4096];
	void                  *module_handle;
	
	module_init_go();

	if((modules_dir = opendir(opt_modules_dir)) == NULL)
		return;
	
	while( (dir = readdir(modules_dir)) != NULL){
		if((ext = strrchr(dir->d_name, '.')) == NULL)
			continue;
		
		if(strcasecmp(ext, ".so") != 0)
			continue;

		if( snprintf(module_path, sizeof(module_path), "%s/%s", opt_modules_dir, dir->d_name) >= sizeof(module_path) )
			continue; // truncated path
		
		dlerror();
		if( (module_handle = dlopen(module_path, RTLD_NOW)) == NULL){
			printf("warning: file '%s' failed: %s\n", dir->d_name, dlerror());
			continue;
		}

		if( module_is_gomodule(module_handle) )
			module_load_go(module_handle);

		if( module_is_cmodule(module_handle) )
		      module_load_c(module_handle);
	}

	closedir(modules_dir);
} // }}}
// }}}

void main_cleanup(void){
	/* cleanup */
	frozen_destroy();
	
	remove_pidfile(opt_pidfile);
}
void main_rest(void){
	int                    sig;
	sigset_t               set;
	
	/* options init */
	if(opt_daemon != 0)
		daemonize();
	
	save_pid(opt_pidfile);
	
	if(frozen_init() != 0){
		fprintf(stderr, "libfrozen init failed\n");
		exit(255);
	}
		
	modules_load();
	
	sigfillset(&set);
	sigprocmask(SIG_BLOCK, &set, NULL);                     // block all signals, main thread will handler that
	
	config_t *config = configs_file_parse(opt_config_file);
	if(config != NULL && shop_new(config) != NULL){
		sigprocmask(SIG_UNBLOCK, &set, NULL);           // enable all signals for this thread
		
		while(sigwait(&set, &sig) == 0){
			switch(sig) {
				case SIGHUP:  break;
				case SIGINT:
				case SIGTERM: main_cleanup(); exit(0); break;
			}
		}
	}else{
		fprintf(stderr, "config file not exist, empty or invalid\n");
		exit(255);
	}
}


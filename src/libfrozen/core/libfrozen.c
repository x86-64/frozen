/**
 * @file libfrozen.c
 * @brief Library main functions
 */

#include "libfrozen.h"
#include <dirent.h>
#include <dlfcn.h>

uintmax_t  inited = 0;
char      *frozen_modules_dir = FROZEN_MODULESDIR;

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

	if((modules_dir = opendir(frozen_modules_dir)) == NULL)
		return;
	
	while( (dir = readdir(modules_dir)) != NULL){
		if((ext = strrchr(dir->d_name, '.')) == NULL)
			continue;
		
		if(strcasecmp(ext, ".so") != 0)
			continue;

		if( snprintf(module_path, sizeof(module_path), "%s/%s", frozen_modules_dir, dir->d_name) >= sizeof(module_path) )
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

typedef struct signal_handler_t {
	int                    sig;
	f_signal_handler       handler;
	void                  *userdata;
} signal_handler_t;

list        frozen_signal_handlers = LIST_INITIALIZER;

static void frozen_signal_handler(int sig){ // {{{
	signal_handler_t      *sh;
	void                  *iter              = NULL;
	
	while( (sh = list_iter_next(&frozen_signal_handlers, &iter))){
		if(sh->sig == sig)
			sh->handler(sig, sh->userdata);
	}
} // }}}

ssize_t frozen_subscribe_signal(int sig, f_signal_handler handler, void *userdata){ // {{{
	signal_handler_t      *sh;
	
	signal(sig, frozen_signal_handler);
	
	if( (sh = malloc(sizeof(signal_handler_t))) == NULL)
		return -ENOMEM;
	
	sh->sig      = sig;
	sh->handler  = handler;
	sh->userdata = userdata;
	list_add(&frozen_signal_handlers, sh);
	return 0;
} // }}}
void    frozen_unsubscribe_signal(int sig, f_signal_handler handler, void *userdata){ // {{{
	signal_handler_t      *sh;
	void                  *iter              = NULL;
	
	while( (sh = list_iter_next(&frozen_signal_handlers, &iter))){
		if(sh->sig == sig && sh->handler == handler && sh->userdata == userdata){
			list_delete(&frozen_signal_handlers, sh);
			free(sh);
			break;
		}
	}
} // }}}

/** @brief Initialize library
 * @return 0 on success
 * @return -1 on error
 */
int frozen_init(void){
	ssize_t                ret;
	
	if(inited == 1)
		return 0;

	srandom(time(NULL));
	
	if( (ret = frozen_data_init()) != 0)
		return ret;
	
	if( (ret = frozen_machine_init()) != 0)
		return ret;
	
	modules_load();
	
	
	inited = 1;
	return 0;
}

/** @brief Destroy library
 * @return 0 on success
 * @return -1 on error
 */
int frozen_destroy(void){
	if(inited == 0)
		return 0;
	
	frozen_machine_destroy();
	frozen_data_destroy();
	
	inited = 0;
	return 0;
}


intmax_t  safe_pow(uintmax_t *res, uintmax_t x, uintmax_t y){
	uintmax_t t;
	
	if(y == 0){ t = 1; goto exit; }
	if(y == 1){ t = x; goto exit; }
	
	t = x;
	while(--y >= 1){
		if(__MAX(uintmax_t) / x <= t)
			return -EINVAL;
		
		t *= x;
	}
exit:
	*res = t;
	return 0;
}

intmax_t safe_mul(uintmax_t *res, uintmax_t x, uintmax_t y){
	if(x == 0 || y == 0){
		*res = 0;
		return 0;
	}
	
	if(__MAX(uintmax_t) / x <= y)
		return -EINVAL;
	
	*res = x * y;
	return 0;
}

intmax_t safe_div(uintmax_t *res, uintmax_t x, uintmax_t y){
	if(y == 0)
		return -EINVAL;
	
	*res = x / y;
	return 0;
}

void * memdup(void *ptr, uintmax_t size){
	void                  *new_ptr;

	if( (new_ptr = malloc(size)) == NULL)
		return NULL;
	memcpy(new_ptr, ptr, size);
	return new_ptr;
}

uintmax_t portable_hash(char *p){
	register char          c;
	register uintmax_t     i                 = 1;
	register uintmax_t     key_val           = 0;
	
	while((c = *p++)){
		key_val += c * i * i;
		i++;
	}
	return key_val;
}

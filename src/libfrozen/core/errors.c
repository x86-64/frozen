#include <libfrozen.h>

#include <errors_list.c>

static void default_log_func(char *format, va_list args){ // {{{ 
	vfprintf(stderr, format, args);
} // }}}

static log_func                current_log_func = &default_log_func;
static uintmax_t               last_emodule     = DYNAMIC_EMODULE_START;
static list                    dynamic_errors   = LIST_INITIALIZER;

static int         err_bsearch_int               (const void *m1, const void *m2){ // {{{
	err_item *mi1 = (err_item *) m1;
	err_item *mi2 = (err_item *) m2;
	return (mi1->errnum - mi2->errnum);
} // }}}

const char *       errors_describe                (intmax_t errnum){ // {{{
	err_item               key;
	err_item              *item;
	err_item              *dynamic_list;
	void                  *iter              = NULL;
	uintmax_t              errnum_pos        = -errnum;
	uintmax_t              errnum_emod       = errnum_pos / ESTEP;
	
	if(errnum_emod == 0){
		if( (key.errmsg = strerror(errnum_pos)) != NULL)
			return key.errmsg;
		
	}else if(errnum_emod < DYNAMIC_EMODULE_START){
		// core errors list
		key.errnum = errnum;
		if((item = bsearch(&key, errs_list,
			errs_list_nelements, errs_list_size,
			&err_bsearch_int)) != NULL
		)
			return item->errmsg;
	}else{
		// dynamic errs list
		while( (dynamic_list = list_iter_next(&dynamic_errors, &iter)) != NULL){
			for(item = dynamic_list; item->errnum != 0; item++){
				if(errnum == item->errnum)
					return item->errmsg;
			}
		}
	}
	return "unknown error";
} // }}}
void               errors_log                     (char *format, ...){ // {{{
	va_list                args;
	va_start(args, format);
	current_log_func(format, args);
	va_end(args);
} // }}}
void               errors_set_log_func           (log_func func){ // {{{
	if(func)
		current_log_func = func;
} // }}}
void               errors_register               (err_item *err_list, uintmax_t *emodule){ // {{{
	err_item              *item;
	uintmax_t              new_emodule       = last_emodule++ * ESTEP;
	
	for(item = err_list; item->errnum != 0; item++){
		item->errnum -= new_emodule;
	}
	
	list_add(&dynamic_errors, err_list);
	*emodule = new_emodule;
} // }}}
ssize_t            errors_is_unix                (intmax_t errnum, intmax_t unix_err){ // {{{
	uintmax_t              errnum_err        = (-errnum) % ESTEP;
	
	return (errnum_err == unix_err) ? 1 : 0;
} // }}}


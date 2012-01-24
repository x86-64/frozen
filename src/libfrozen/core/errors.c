#include <libfrozen.h>

#include <errors_list.c>

/*
extern err_item    errs_list[];
extern uintmax_t   errs_list_size
extern uintmax_t   errs_list_nelements
*/

static void default_log_func(char *format, va_list args){ // {{{ 
	vfprintf(stderr, format, args);
} // }}}

static log_func                current_log_func = &default_log_func;

static int         err_bsearch_int               (const void *m1, const void *m2){ // {{{
	err_item *mi1 = (err_item *) m1;
	err_item *mi2 = (err_item *) m2;
	return (mi1->errnum - mi2->errnum);
} // }}}
const char *       describe_error                (intmax_t errnum){ // {{{
	err_item  key, *ret;
	key.errnum = errnum;
	
	if((ret = bsearch(&key, errs_list,
		errs_list_nelements, errs_list_size,
		&err_bsearch_int)) == NULL)
		return "unknown error"; //NULL;
	
	return ret->errmsg;
} // }}}
intmax_t           handle_error                  (uintmax_t eflag, intmax_t errnum){ // {{{
	return errnum;
} // }}}
void               log_error                     (char *format, ...){ // {{{
	va_list                args;
	va_start(args, format);
	current_log_func(format, args);
	va_end(args);
} // }}}
void               set_log_func                  (log_func func){ // {{{
	if(func)
		current_log_func = func;
} // }}}

/* vim: set filetype=c: */

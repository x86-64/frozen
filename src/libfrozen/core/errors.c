#include <libfrozen.h>

#include <errors_list.c>

/*
extern err_item    errs_list[];
extern uintmax_t   errs_list_size
extern uintmax_t   errs_list_nelements
*/

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
        const char *errmsg;
	// check global_settings
 	       
	if( (errmsg = describe_error(errnum)) == NULL)
		goto exit;
	
	fprintf(stderr, "%s\n", errmsg);
exit:
	return errnum;
} // }}}

/* vim: set filetype=c: */

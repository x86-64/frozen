#include <libfrozen.h>
#include <pcre.h>
#include <pcre_t.h>

#include <errors_list.c>

typedef struct pcre_t {
	data_t                *data;
	data_t                 freeit;
	
	pcre                  *re;
	int                    options;
} pcre_t;

static void    pcre_t_destroy(pcre_t *fdata){ // {{{
	data_free(&fdata->freeit);
	pcre_free(fdata->re);
	free(fdata);
} // }}}
static ssize_t pcre_t_new(pcre_t **pfdata, config_t *config){ // {{{
	ssize_t                ret;
	char                  *cfg_regexp        = NULL;
	int                    options           = 0;
	int                    erroffset;
	const char            *errmsg;
	pcre_t                *fdata;
	
	if( (fdata = calloc(1, sizeof(pcre_t))) == NULL)
		return error("calloc returns null");
	
	hash_data_convert(ret, TYPE_STRINGT, cfg_regexp, config, HDK(regexp));
	if(ret != 0){
		ret = error("regexp not supplied");
		goto error;
	}
	
	if( (fdata->re = pcre_compile(
		cfg_regexp,
		options,
		&errmsg,
		&erroffset,
		NULL)) == NULL
	){
		ret = error("regexp compile failed");
		goto error;
	}
	
	fdata->options = 0;
	fdata->data    = NULL;
	data_set_void(&fdata->freeit);
	
	*pfdata = fdata;
	free(cfg_regexp);
	return 0;
	
error:
	free(fdata);
	return ret;
} // }}}

static ssize_t data_pcre_t_nosys(data_t *data, fastcall_header *hargs){ // {{{
	return -ENOSYS;
} // }}}
static ssize_t data_pcre_t_convert_from(data_t *dst, fastcall_convert_from *fargs){ // {{{
	ssize_t                ret;
	pcre_t                *fdata;
	
	if(fargs->src == NULL)
		return -EINVAL;
	
	switch( fargs->format ){
		case FORMAT(hash):;
			hash_t                *config;
			data_t                 data;
			
			data_get(ret, TYPE_HASHT, config, fargs->src);
			if(ret != 0)
				return -EINVAL;
			
			if( (ret = pcre_t_new((pcre_t **)&dst->ptr, config)) < 0)
				return ret;
			
			fdata = (pcre_t *)dst->ptr;
			
			hash_holder_consume(ret, data, config, HDK(data));
			if(ret == 0){
				fdata->freeit = data;
				fdata->data   = &fdata->freeit;
			}
			return 0;

		case FORMAT(config):;
		case FORMAT(human):;
			request_t r_config[] = {
				{ HDK(regexp), *fargs->src },
				hash_end
			};
			return pcre_t_new((pcre_t **)&dst->ptr, r_config);
		
		default:
			break;
	};
	return -ENOSYS;
} // }}}
static ssize_t data_pcre_t_free(data_t *data, fastcall_free *fargs){ // {{{
	pcre_t                  *fdata             = (pcre_t *)data->ptr;
	
	pcre_t_destroy(fdata);
	return 0;
} // }}}
static ssize_t data_pcre_t_enum(data_t *data, fastcall_enum *fargs){ // {{{
	ssize_t                ret;
	int                    ovector[1 * 3];
	void                  *data_ptr;
	uintmax_t              data_size;
	data_t                 freeit;
	uintmax_t              offset            = 0;
	pcre_t                *fdata             = (pcre_t *)data->ptr;
	
	if(fdata->data == NULL)
		return errorn(ENOSYS);
	
	if( (ret = data_make_flat(fdata->data, FORMAT(native), &freeit, &data_ptr, &data_size)) < 0){
		data_free(&freeit);
		return ret;
	}
	
	// deref fdata->data
	fastcall_getdata r_getdata = { { 3, ACTION_GETDATA } };
	if( (ret = data_query(fdata->data, &r_getdata)) < 0){
		data_free(&freeit);
		return ret;
	}
	
	while(1){
		if(pcre_exec(
			fdata->re, NULL,
			data_ptr, data_size,
			offset,
			fdata->options,
			ovector,
			(sizeof(ovector) / sizeof(ovector[0]))
		) < 0)
			goto exit;
		
		data_t        d_item      = DATA_HEAP_SLICET(r_getdata.data, ovector[0], ovector[1] - ovector[0]);
		fastcall_push r_push_item = { { 3, ACTION_PUSH }, &d_item };
		ret = data_query(fargs->dest, &r_push_item);
		
		data_free(&d_item);
		if(ret < 0)
			goto exit;
		
		offset = ovector[1];
	}
	ret = 0;
	
exit:;
	fastcall_push r_push_end = { { 3, ACTION_PUSH }, NULL };
	data_query(fargs->dest, &r_push_end);
	
	data_free(&freeit);
	return ret;
} // }}}
static ssize_t data_pcre_t_compare(data_t *data, fastcall_compare *fargs){ // {{{
	ssize_t                ret;
	int                    ovector[1 * 3];
	void                  *data_ptr;
	uintmax_t              data_size;
	data_t                 freeit;
	pcre_t                *fdata             = (pcre_t *)data->ptr;
	
	if(fargs->data2 == NULL)
		return errorn(ENOSYS);
	
	if( (ret = data_make_flat(fargs->data2, FORMAT(native), &freeit, &data_ptr, &data_size)) < 0){
		data_free(&freeit);
		return ret;
	}
	
	if(pcre_exec(
		fdata->re, NULL,
		data_ptr, data_size,
		0,
		fdata->options,
		ovector,
		(sizeof(ovector) / sizeof(ovector[0]))
	) < 0){
		fargs->result = 1; // not match
	}else{
		fargs->result = 0; // match
	}
	
	data_free(&freeit);
	return 0;
} // }}}

data_proto_t pcre_t_proto = {
	.type_str               = PCRET_NAME,
	.api_type               = API_HANDLERS,
	.handlers               = {
		[ACTION_LENGTH]         = (f_data_func)&data_pcre_t_nosys,
		[ACTION_COMPARE]        = (f_data_func)&data_pcre_t_compare,
		[ACTION_CONVERT_FROM]   = (f_data_func)&data_pcre_t_convert_from,
		[ACTION_FREE]           = (f_data_func)&data_pcre_t_free,
		[ACTION_ENUM]           = (f_data_func)&data_pcre_t_enum,
	}
};

int main(void){
	errors_register((err_item *)&errs_list, &emodule);
	data_register(&pcre_t_proto);
	
	type_pcret = datatype_t_getid_byname(PCRET_NAME, NULL);
	return 0;
}


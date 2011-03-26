#include <libfrozen.h>
#include <void_t.h>

size_t  data_void_len(data_t *data, data_ctx_t *ctx){
	(void)data; (void)ctx;
	return 0;;
}
size_t  data_void_rawlen(data_t *data, data_ctx_t *ctx){
	(void)data; (void)ctx;
	return 0;
}
size_t data_void_len2raw(size_t unitsize){
	(void)unitsize;
	return 0;
}

ssize_t data_void_convert(data_t *dst, data_ctx_t *dst_ctx, data_t *src, data_ctx_t *src_ctx){ // {{{
	(void)dst; (void)dst_ctx; (void)src; (void)src_ctx;
	return 0;
} // }}}


data_proto_t void_t_proto = {
		.type          = TYPE_VOID,
		.type_str      = "void_t",
		.size_type     = SIZE_VARIABLE,
		.func_len      = &data_void_len,
		.func_rawlen   = &data_void_rawlen,
		.func_len2raw  = &data_void_len2raw,
		.func_convert  = &data_void_convert
};


#include <libfrozen.h>
#include <raw_t.h>

size_t  data_raw_len(data_t *data, data_ctx_t *ctx){
	(void)ctx; // TODO add ctx parse
	return data->data_size;
}

data_proto_t raw_t_proto = {
		.type          = TYPE_RAW,
		.type_str      = "raw_t",
		.size_type     = SIZE_VARIABLE,
		.func_len      = &data_raw_len,
};


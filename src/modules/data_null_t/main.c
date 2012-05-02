#include <libfrozen.h>

ssize_t mynull_handler(data_t *data, fastcall_header *hargs){
	printf("mynull_t called\n");
	return 0;
}

data_proto_t mynull_proto = {
	.type_str               = "mynull_t",
	.api_type               = API_HANDLERS,
	.handler_default        = (f_data_func)&mynull_handler
};

int main(void){
	data_register(&mynull_proto);
	return 0;
}

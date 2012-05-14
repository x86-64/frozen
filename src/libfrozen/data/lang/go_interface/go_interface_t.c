#include <libfrozen.h>
#include <go_interface_t.h>

data_proto_t go_interface_t_proto = {
	.type                   = TYPE_GOINTERFACET,
	.type_str               = "gointerface_t",
	.api_type               = API_HANDLERS,
	.properties             = DATA_ENDPOINT,
};

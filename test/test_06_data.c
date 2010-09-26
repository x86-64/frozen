#include "test.h"

START_TEST (test_data){
	int i;
	for(i=0; i<data_types_size; i++){
		fail_unless(data_types[i].type == i, "data_types have invalid items order");
		fail_unless(data_types[i].type == data_type_from_string(data_types[i].type_str), "data_types have invalid string names");
	}
}
END_TEST
REGISTER_TEST(core, test_data)


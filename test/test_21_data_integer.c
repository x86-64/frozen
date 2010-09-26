#include "test.h"

START_TEST (test_data_integer){
	unsigned long long  v100_1 = 0x11223344;
	unsigned long long  v100_2 = 0x11223344;
	unsigned long long  v200_1 = 0x44556677;
	
	fail_unless(data_protos[TYPE_INT8].fixed_size  == 1, "data_int8  fixed_size failed");
	fail_unless(data_protos[TYPE_INT16].fixed_size == 2, "data_int16 fixed_size failed");
	fail_unless(data_protos[TYPE_INT32].fixed_size == 4, "data_int32 fixed_size failed");
	fail_unless(data_protos[TYPE_INT64].fixed_size == 8, "data_int64 fixed_size failed");
	
	fail_unless(data_protos[TYPE_INT8].func_cmp  == &data_int8_cmp,  "data_int8_cmp func failed");
	fail_unless(data_protos[TYPE_INT16].func_cmp == &data_int16_cmp, "data_int16_cmp func failed");
	fail_unless(data_protos[TYPE_INT32].func_cmp == &data_int32_cmp, "data_int32_cmp func failed");
	fail_unless(data_protos[TYPE_INT64].func_cmp == &data_int64_cmp, "data_int64_cmp func failed");
	
	fail_unless(data_int8_cmp(&v100_1, &v100_2) ==  0, "data_int8_cmp cmp failed 1");
	fail_unless(data_int8_cmp(&v100_1, &v200_1) == -1, "data_int8_cmp cmp failed 2");
	fail_unless(data_int8_cmp(&v200_1, &v100_1) ==  1, "data_int8_cmp cmp failed 3");
	
	fail_unless(data_int16_cmp(&v100_1, &v100_2) ==  0, "data_int16_cmp cmp failed 1");
	fail_unless(data_int16_cmp(&v100_1, &v200_1) == -1, "data_int16_cmp cmp failed 2");
	fail_unless(data_int16_cmp(&v200_1, &v100_1) ==  1, "data_int16_cmp cmp failed 3");
	
	fail_unless(data_int32_cmp(&v100_1, &v100_2) ==  0, "data_int32_cmp cmp failed 1");
	fail_unless(data_int32_cmp(&v100_1, &v200_1) == -1, "data_int32_cmp cmp failed 2");
	fail_unless(data_int32_cmp(&v200_1, &v100_1) ==  1, "data_int32_cmp cmp failed 3");
	
	fail_unless(data_int64_cmp(&v100_1, &v100_2) ==  0, "data_int64_cmp cmp failed 1");
	fail_unless(data_int64_cmp(&v100_1, &v200_1) == -1, "data_int64_cmp cmp failed 2");
	fail_unless(data_int64_cmp(&v200_1, &v100_1) ==  1, "data_int64_cmp cmp failed 3");
}
END_TEST
REGISTER_TEST(core, test_data_integer)


#include "test.h"

START_TEST (test_data_integer){
	unsigned long long  v100_1 = 0x11223344;
	unsigned long long  v100_2 = 0x11223344;
	unsigned long long  v200_1 = 0x44556677;
	
	fail_unless(int8_cmp_func(&v100_1, &v100_2) ==  0, "int8_cmp_func cmp failed 1");
	fail_unless(int8_cmp_func(&v100_1, &v200_1) == -1, "int8_cmp_func cmp failed 2");
	fail_unless(int8_cmp_func(&v200_1, &v100_1) ==  1, "int8_cmp_func cmp failed 3");
	
	fail_unless(int16_cmp_func(&v100_1, &v100_2) ==  0, "int16_cmp_func cmp failed 1");
	fail_unless(int16_cmp_func(&v100_1, &v200_1) == -1, "int16_cmp_func cmp failed 2");
	fail_unless(int16_cmp_func(&v200_1, &v100_1) ==  1, "int16_cmp_func cmp failed 3");
	
	fail_unless(int32_cmp_func(&v100_1, &v100_2) ==  0, "int32_cmp_func cmp failed 1");
	fail_unless(int32_cmp_func(&v100_1, &v200_1) == -1, "int32_cmp_func cmp failed 2");
	fail_unless(int32_cmp_func(&v200_1, &v100_1) ==  1, "int32_cmp_func cmp failed 3");
	
	fail_unless(int64_cmp_func(&v100_1, &v100_2) ==  0, "int64_cmp_func cmp failed 1");
	fail_unless(int64_cmp_func(&v100_1, &v200_1) == -1, "int64_cmp_func cmp failed 2");
	fail_unless(int64_cmp_func(&v200_1, &v100_1) ==  1, "int64_cmp_func cmp failed 3");
}
END_TEST
REGISTER_TEST(core, test_data_integer)


#include <test_00_settings.c>
#include <test_05_buffer.c>
#include <test_06_data.c>
#include <test_07_data_integer.c>
#include <test_09_structs.c>
#include <test_10_hash.c>
#include <test_11_backend_file.c>
#include <test_19_backend.c>

void test_list(void){
	tcase_add_test(tc_core, test_settings_basic);
	tcase_add_test(tc_core, test_settings_strings);
	tcase_add_test(tc_core, test_settings_childs);
	tcase_add_test(tc_core, test_buffer);
	tcase_add_test(tc_core, test_data);
	tcase_add_test(tc_core, test_data_integer);
	tcase_add_test(tc_core, test_structs);
	tcase_add_test(tc_core, test_hash);
	tcase_add_test(tc_core, test_backend_file);
	tcase_add_test(tc_core, test_backends);
	tcase_add_test(tc_core, test_backends_two_chains);
}

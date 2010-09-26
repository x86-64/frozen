#include <test.h>

Suite *s;
TCase *tc_core;

static void __attribute__ ((constructor)) __test_init(){
	s       = suite_create ("frozen");
	tc_core = tcase_create ("core");
}

int main (void){
	int number_failed;
	
	suite_add_tcase (s, tc_core);
	
	SRunner *sr = srunner_create (s);
	srunner_run_all (sr, CK_VERBOSE);
	number_failed = srunner_ntests_failed (sr);
	srunner_free (sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

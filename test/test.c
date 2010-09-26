#include <test.h>

Suite *s;
TCase *tc_core;

#include <tests_list.c>

int main (void){
	int number_failed;
	
	frozen_init();
	
	s       = suite_create ("frozen");
	tc_core = tcase_create ("core");
	test_list();
	suite_add_tcase (s, tc_core);
	
	SRunner *sr = srunner_create (s);
	srunner_set_fork_status(sr, CK_NOFORK);
	srunner_run_all (sr, CK_VERBOSE);
	number_failed = srunner_ntests_failed (sr);
	srunner_free (sr);
	
	frozen_destory();
	
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

#include "test.h"

START_TEST (test_backends){
	backend_t *backend, *found;
	
	setting_set_child_string(global_settings, "homedir", ".");
	
	setting_t *settings = setting_new();
		setting_t *s_file = setting_new();
			setting_set_child_string(s_file, "name",     "file");
	setting_set_child_setting(settings, s_file);
	
	
	/* run with wrong parameters */
	backend = backend_new("in_file", settings);
		fail_unless(backend == NULL, "backend with wrong options created");
	
	/* fix parameters */
	setting_set_child_string(s_file, "filename", "data_backend_file");
	
	/* re-run with good parameters */
	backend = backend_new("in_file", settings);
		fail_unless(backend != NULL, "backend creation failed");
	
	found = backend_find_by_name("in_file");
		fail_unless(backend == found, "backend_find_by_name not working");
	
	backend_destroy(backend);
	
	setting_destroy(settings);
}
END_TEST
REGISTER_TEST(core, test_backends)


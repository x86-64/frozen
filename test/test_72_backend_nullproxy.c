#include <test.h>

START_TEST (test_backends_two_chains){
	backend_t *backend;
	
	setting_set_child_string(global_settings, "homedir", ".");
	
	setting_t *settings = setting_new();
		setting_t *s_file = setting_new();
			setting_set_child_string(s_file, "name",     "file");
			setting_set_child_string(s_file, "filename", "data_backend_file");
		setting_t *s_null = setting_new();
			setting_set_child_string(s_null, "name",     "null-proxy");
	setting_set_child_setting(settings, s_file);
	setting_set_child_setting(settings, s_null);
	
	/* create backend */
	backend = backend_new("in_file", settings);
		fail_unless(backend != NULL, "backend creation failed");
	
	/* test read/writes */
	buffer_t       *buffer  = buffer_alloc();
	ssize_t         ssize;
	hash_t          request[] = {
		{ TYPE_INT32, "action",  (int []) { ACTION_CRWD_CREATE } },
		{ TYPE_INT32, "size",    (int []) { 0xBEEF }             },
		hash_end
	};
	
	ssize = backend_query(backend, request, buffer);
		fail_unless(ssize == 0x0000BEEF, "backend chain incorrectly set");
		
	buffer_free(buffer);
	
	// rest will work, won't test
	
	backend_destroy(backend);
	setting_destroy(settings);
}
END_TEST
REGISTER_TEST(core, test_backends_two_chains)


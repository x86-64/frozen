#include "test.h"
#include "../src/backend.h"

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
	
	backend_destory(backend);
	
	setting_destroy(settings);
}
END_TEST
REGISTER_TEST(core, test_backends)

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
	int    ret;
	off_t  key;
	buffer_t *test1_val = buffer_from_data("test167890", 10);
	buffer_t *test_buff = buffer_alloc();
	void     *test1_chunk = buffer_get_first_chunk(test1_val);
	void     *test_chunk;
	
	ret = backend_crwd_create(backend, &key, 10);
		fail_unless(ret == 0, "backend key create failed");
	ret = backend_crwd_create(backend, &key, 0x0000BEEF);
		fail_unless(ret == 0x0000BEEF, "backend chain incorrectly set");
	ret = backend_crwd_set(backend, &key, test1_val, 10);
		fail_unless(ret == 0, "backend key set failed");
	ret = backend_crwd_get(backend, &key, test_buff, 10);
	test_chunk = buffer_get_first_chunk(test_buff);
		fail_unless(ret == 0 && strncmp(test1_chunk, test_chunk, 10) == 0, "backend key get failed");
	ret = backend_crwd_delete(backend, &key, 10);
		fail_unless(ret == 0, "backend key delete failed");
	
	buffer_free(test1_val);
	buffer_free(test_buff);

	backend_destory(backend);
	
	setting_destroy(settings);
}
END_TEST
REGISTER_TEST(core, test_backends_two_chains)


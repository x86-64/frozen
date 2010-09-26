#include "test.h"

START_TEST (test_backend_locator){
	backend_t *backend;
	
	setting_set_child_string(global_settings, "homedir", ".");
	
	setting_t *settings = setting_new();
		setting_t *s_file = setting_new();
			setting_set_child_string(s_file, "name",        "file");
			setting_set_child_string(s_file, "filename",    "data_backend_locator");
		setting_t *s_loca = setting_new();
			setting_set_child_string(s_loca, "name",        "locator");
			setting_set_child_string(s_loca, "struct_size", "16");
	setting_set_child_setting(settings, s_file);
	setting_set_child_setting(settings, s_loca);
	
	/* create backend */
	backend = backend_new("in_locator", settings);
		fail_unless(backend != NULL, "backend creation failed");
	
	/* test read/writes */
	int    ret;
	off_t  key;
	buffer_t *test1_val = buffer_from_data("test167890123456", 16);
	buffer_t *test_buff = buffer_alloc();
	void     *test1_chunk = buffer_get_first_chunk(test1_val);
	void     *test_chunk;
	
	ret = backend_crwd_create(backend, &key, 1);
		fail_unless(ret == 0, "backend key create failed");
	ret = backend_crwd_set(backend, &key, test1_val, 1);
		fail_unless(ret == 0, "backend key set failed");
	ret = backend_crwd_get(backend, &key, test_buff, 1);
	test_chunk = buffer_get_first_chunk(test_buff);
		fail_unless(ret == 0 && strncmp(test1_chunk, test_chunk, 16) == 0, "backend key get failed");
	ret = backend_crwd_delete(backend, &key, 1);
		fail_unless(ret == 0, "backend key delete failed");
	
	buffer_free(test1_val);
	buffer_free(test_buff);

	backend_destory(backend);
	setting_destroy(settings);
}
END_TEST
REGISTER_TEST(core, test_backend_locator)


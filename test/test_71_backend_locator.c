#include "test.h"

START_TEST (test_backend_locator){
	backend_t *backend;
	
	setting_set_child_string(global_settings, "homedir", ".");
	
	setting_t *settings = setting_new();
		setting_t *s_file = setting_new();
			setting_set_child_string(s_file, "name",        "file");
			setting_set_child_string(s_file, "filename",    "data_backend_locator");
		setting_t *s_loca = setting_new();
			setting_set_child_string(s_loca, "name",        "oid-locator");
			setting_set_child_string(s_loca, "mode",        "linear-incapsulated");
			setting_set_child_string(s_loca, "oid-class",   "int32");
			setting_set_child_string(s_loca, "size",        "16");
			
	setting_set_child_setting(settings, s_file);
	setting_set_child_setting(settings, s_loca);
	
	backend = backend_new("in_locator", settings);
		fail_unless(backend != NULL, "backend creation failed");

	unsigned int  action;
	hash_t       *hash;
	ssize_t       ssize;
	off_t         new_key1, new_key2;
	buffer_t     *buffer = buffer_alloc();
	char  key1_data[]  = "\x0e\x00\x00\x00test167890";
	char  key2_data[]  = "\x0e\x00\x00\x00test267890";
	
	action  = ACTION_CRWD_CREATE;
	hash = hash_new();
		hash_set(hash, "action", TYPE_INT32, &action);
		hash_set(hash, "size",   TYPE_INT32, "\x01\x00\x00\x00");
		
		if( (ssize = backend_query(backend, hash, buffer)) != sizeof(off_t) )
			fail("chain file create key1 failed");	
		buffer_read(buffer, ssize, new_key1 = *(off_t *)chunk; break);
		
		hash_set(hash, "size",   TYPE_INT32, "\x01\x00\x00\x00");
		
		if( (ssize = backend_query(backend, hash, buffer)) != sizeof(off_t) )
			fail("chain file create key2 failed");	
		buffer_read(buffer, ssize, new_key2 = *(off_t *)chunk; break);
			
			fail_unless(new_key2 - new_key1 == 1,                 "backend in_locator offsets invalid");
	
	action  = ACTION_CRWD_WRITE;
		hash_set(hash, "action", TYPE_INT32,  &action);
		hash_set(hash, "key",    TYPE_INT64,  &new_key1);
		hash_set(hash, "value",  TYPE_BINARY, &key1_data);
		
		ssize = backend_query(backend, hash, buffer);
			fail_unless(ssize == 1,  "backend in_locator write failed");
		
		
	hash_free(hash);
	buffer_free(buffer);

/*
	ret = backend_crwd_get(backend, &key, test_buff, 1);
	test_chunk = buffer_get_first_chunk(test_buff);
		fail_unless(ret == 0 && strncmp(test1_chunk, test_chunk, 16) == 0, "backend key get failed");
	ret = backend_crwd_delete(backend, &key, 1);
		fail_unless(ret == 0, "backend key delete failed");
*/

	backend_destory(backend);
	setting_destroy(settings);
}
END_TEST
REGISTER_TEST(core, test_backend_locator)


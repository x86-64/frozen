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
	char  *test_chunk;
	char  key1_data[]  = "test167890";
	char  key2_data[]  = "test267890";
	data_t *data_buf;

	hash = hash_new();
	
	action  = ACTION_CRWD_CREATE;
	ssize   = 1;
		hash_set(hash, "action", TYPE_INT32, &action);
		hash_set(hash, "size",   TYPE_INT32,  &ssize);
		
		if( (ssize = backend_query(backend, hash, buffer)) != sizeof(off_t) )
			fail("chain file create key1 failed");	
		buffer_read(buffer, 0, &new_key1, MIN(ssize, sizeof(off_t)));
		
		hash_set(hash, "size",   TYPE_INT32,  &ssize);
		
		if( (ssize = backend_query(backend, hash, buffer)) != sizeof(off_t) )
			fail("chain file create key2 failed");	
		buffer_read(buffer, 0, &new_key2, MIN(ssize, sizeof(off_t)));
			
			fail_unless(new_key2 - new_key1 == 1,                 "backend in_locator offsets invalid");
	
	action  = ACTION_CRWD_WRITE;
	ssize   = 1;
		hash_set(hash, "action", TYPE_INT32,  &action);
		hash_set(hash, "key",    TYPE_INT64,  &new_key1);
		hash_set(hash, "size",   TYPE_INT32,  &ssize);
		
		buffer_write(buffer, 0, &key1_data, 10);
		
		ssize = backend_query(backend, hash, buffer);
			fail_unless(ssize == 1,  "backend in_locator write 1 failed");
	
	hash_empty(hash);
	ssize   = 1;
		hash_set(hash, "action", TYPE_INT32,  &action);
		hash_set(hash, "key",    TYPE_INT64,  &new_key2);
		hash_set(hash, "size",   TYPE_INT32,  &ssize);
		
		buffer_write(buffer, 0, &key2_data, 10);
		
		ssize = backend_query(backend, hash, buffer);
			fail_unless(ssize == 1,  "backend in_locator write 2 failed");
	
	action  = ACTION_CRWD_READ;
	ssize   = 1;
		hash_set(hash, "action", TYPE_INT32,  &action);
		hash_set(hash, "key",    TYPE_INT64,  &new_key1);
		hash_set(hash, "size",   TYPE_INT32,  &ssize);
		
		buffer_remove_chunks(buffer);
		
		ssize = backend_query(backend, hash, buffer);
			fail_unless(ssize == 1,                             "backend in_locator read 1 failed");
			
		test_chunk = buffer_get_first_chunk(buffer);
			fail_unless(
				test_chunk != NULL && memcmp(test_chunk, key1_data, 10) == 0,
				"backend in_locator read 1 data failed"
			);
	
	ssize   = 1;
		hash_set(hash, "key",    TYPE_INT64,  &new_key2);
		hash_set(hash, "size",   TYPE_INT32,  &ssize);
		
		buffer_remove_chunks(buffer);
		
		ssize = backend_query(backend, hash, buffer);
			fail_unless(ssize == 1,                             "backend in_locator read 2 failed");
			
		test_chunk = buffer_get_first_chunk(buffer);
			fail_unless(
				test_chunk != NULL &&
				memcmp(test_chunk, key2_data, 10) == 0,
				"backend in_locator read 2 data failed"
			);
		
	hash_free(hash);
	buffer_free(buffer);

	backend_destory(backend);
	setting_destroy(settings);
}
END_TEST
REGISTER_TEST(core, test_backend_locator)


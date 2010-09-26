#include "test.h"

START_TEST (test_backend_blocks){
	backend_t *backend;
	
	setting_set_child_string(global_settings, "homedir", ".");
	
	setting_t *settings = setting_new();
		setting_t *s_file = setting_new();
			setting_set_child_string(s_file, "name",        "file");
			setting_set_child_string(s_file, "filename",    "data_backend_blocks");
		setting_t *s_list = setting_new();
			setting_set_child_string(s_list, "name",        "blocks");
			setting_set_child_string(s_list, "block_size",  "5");
				setting_t *s_backend = setting_new_named("backend");
					setting_t *s_bfile = setting_new();
						setting_set_child_string(s_bfile, "name",        "file");
						setting_set_child_string(s_bfile, "filename",    "data_backend_blocks_map");
					setting_t *s_bloca = setting_new();
						setting_set_child_string(s_bloca, "name",        "oid-locator");
						setting_set_child_string(s_bloca, "mode",        "linear-incapsulated");
						setting_set_child_string(s_bloca, "oid-class",   "int32");
						setting_set_child_string(s_bloca, "size",        "8");
				setting_set_child_setting(s_backend, s_bfile);
				setting_set_child_setting(s_backend, s_bloca);
			setting_set_child_setting(s_list, s_backend);
	setting_set_child_setting(settings, s_file);
	setting_set_child_setting(settings, s_list);
	
	backend = backend_new("in_block", settings);
		fail_unless(backend != NULL, "backend creation failed");
	
	unsigned int  action;
	ssize_t       ssize;
	char         *test_chunk;
	buffer_t     *buffer = buffer_alloc();
	hash_t       *hash   = hash_new();
	
	int           k;
	int           key_count = 20;
	off_t         key_off[key_count];
	
	for(k=0; k<key_count; k++){
		action  = ACTION_CRWD_CREATE;
		ssize   = 1;
		
		hash_set(hash, "action", TYPE_INT32, &action);
		hash_set(hash, "size",   TYPE_INT32,  &ssize);
		if( (ssize = backend_query(backend, hash, buffer)) != sizeof(off_t) )
			fail("chain in_block create failed");	
		
		buffer_read(buffer, ssize, key_off[k] = *(off_t *)chunk; break);
		printf("new_key[%x]: %x\n", (unsigned int)k, (unsigned int)key_off[k]);
	}
	
	/*
	action  = ACTION_CRWD_WRITE;
	ssize   = 10;
		hash_set(hash, "action", TYPE_INT32,  &action);
		hash_set(hash, "key",    TYPE_INT64,  &key_off);
		hash_set(hash, "value",  TYPE_BINARY, &key_data);
		hash_set(hash, "size",   TYPE_INT32,  &ssize);
		
		ssize = backend_query(backend, hash, buffer);
			fail_unless(ssize == 10, "backend in_list write 1 failed");
	
	// insert key
	key_insert_off = key_off + 2;
	ssize   = 1;
		hash_set(hash, "key",    TYPE_INT64,  &key_insert_off);
		hash_set(hash, "value",  TYPE_BINARY, &key_insert);
		hash_set(hash, "insert", TYPE_INT32,  &ssize); // = 1
		hash_set(hash, "size",   TYPE_INT32,  &ssize); // = 1
		
		ssize = backend_query(backend, hash, buffer);
			fail_unless(ssize == 1,  "backend in_list write 2 failed");
	
	// check
	action  = ACTION_CRWD_READ;
	ssize   = 11;
		hash_set(hash, "action", TYPE_INT32,  &action);
		hash_set(hash, "key",    TYPE_INT64,  &key_off);
		hash_set(hash, "size",   TYPE_INT32,  &ssize);
		
		buffer_remove_chunks(buffer);
		
		ssize = backend_query(backend, hash, buffer);
			fail_unless(ssize == 11,                                "backend in_list read 1 failed");
			
		test_chunk = buffer_get_first_chunk(buffer);
			fail_unless(memcmp(test_chunk, key_inserted, 11) == 0, "backend in_list read 1 data failed");
	*/

	hash_free(hash);
	buffer_free(buffer);

	backend_destory(backend);
	setting_destroy(settings);
}
END_TEST
REGISTER_TEST(core, test_backend_blocks)


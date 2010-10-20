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
	
	ssize_t       ssize;
	off_t         new_key1, new_key2;
	buffer_t     *buffer = buffer_alloc();
	
	char          key1_data[]  = "test167890";
	char          key2_data[]  = "test267890";
	buffer_t     *key1_buffer  = buffer_alloc_from_bare(&key1_data, sizeof(key1_data));
	buffer_t     *key2_buffer  = buffer_alloc_from_bare(&key2_data, sizeof(key2_data));
	
	hash_t  hash_create1[] = {
		{ "action", DATA_INT32(ACTION_CRWD_CREATE) },
		{ "size",   DATA_INT32(1)                  },
		hash_end
	};
	if( (ssize = backend_query(backend, hash_create1, buffer)) != sizeof(off_t) )
		fail("chain file create key1 failed");	
	buffer_read(buffer, 0, &new_key1, MIN(ssize, sizeof(off_t)));
		
	hash_t  hash_create2[] = {
		{ "action", DATA_INT32(ACTION_CRWD_CREATE) },
		{ "size",   DATA_INT32(1)                  },
		hash_end
	};
	if( (ssize = backend_query(backend, hash_create2, buffer)) != sizeof(off_t) )
		fail("chain file create key2 failed");	
	buffer_read(buffer, 0, &new_key2, MIN(ssize, sizeof(off_t)));
		
		fail_unless(new_key2 - new_key1 == 1,                 "backend in_locator offsets invalid");
	

	hash_t  hash_write1[] = {
		{ "action", DATA_INT32(ACTION_CRWD_WRITE) },
		{ "key",    DATA_OFFT(new_key1)           },
		{ "size",   DATA_INT32(1)                 },
		hash_end
	};
	buffer_write(buffer, 0, &key1_data, 10);
	ssize = backend_query(backend, hash_write1, buffer);
		fail_unless(ssize == 1,  "backend in_locator write 1 failed");
	
	
	hash_t  hash_write2[] = {
		{ "action", DATA_INT32(ACTION_CRWD_WRITE) },
		{ "key",    DATA_OFFT(new_key2)           },
		{ "size",   DATA_INT32(1)                 },
		hash_end
	};
	buffer_write(buffer, 0, &key2_data, 10);
	ssize = backend_query(backend, hash_write2, buffer);
		fail_unless(ssize == 1,  "backend in_locator write 2 failed");
	
	
	hash_t  hash_read1[] = {
		{ "action", DATA_INT32(ACTION_CRWD_READ)  },
		{ "key",    DATA_OFFT(new_key1)           },
		{ "size",   DATA_INT32(1)                 },
		hash_end
	};
	buffer_cleanup(buffer);
	ssize = backend_query(backend, hash_read1, buffer);
		fail_unless(ssize == 1,                             "backend in_locator read 1 failed");
		fail_unless(
			buffer_memcmp(buffer, 0, key1_buffer, 0, 10) == 0,
			"backend in_locator read 1 data failed"
		);
	
	hash_t  hash_read2[] = {
		{ "action", DATA_INT32(ACTION_CRWD_READ) },
		{ "key",    DATA_OFFT(new_key2)          },
		{ "size",   DATA_INT32(1)                },
		hash_end
	};
	buffer_cleanup(buffer);
		
	ssize = backend_query(backend, hash_read2, buffer);
		fail_unless(ssize == 1,                             "backend in_locator read 2 failed");
		fail_unless(
			buffer_memcmp(buffer, 0, key2_buffer, 0, 10) == 0,
			"backend in_locator read 2 data failed"
		);
	
	buffer_free(buffer);
	
	buffer_free(key1_buffer);
	buffer_free(key2_buffer);
	
	backend_destroy(backend);
	setting_destroy(settings);
}
END_TEST
REGISTER_TEST(core, test_backend_locator)


#include "test.h"

START_TEST (test_backend_list){
	backend_t *backend;
	
	setting_set_child_string(global_settings, "homedir", ".");
	
	setting_t *settings = setting_new();
		setting_t *s_file = setting_new();
			setting_set_child_string(s_file, "name",        "file");
			setting_set_child_string(s_file, "filename",    "data_backend_list");
	setting_t *s_list = setting_new();
			setting_set_child_string(s_list, "name",        "list");
	setting_set_child_setting(settings, s_file);
	setting_set_child_setting(settings, s_list);
	
	backend = backend_new("in_list", settings);
		fail_unless(backend != NULL, "backend creation failed");
	
	ssize_t       ssize;
	buffer_t     *buffer = buffer_alloc();
	
	off_t         key_off;
	char          key_data[]     = "0123456789";
	char          key_insert[]   = "a";
	char          key_inserted[] = "01a23456789";
	char          key_delete[]   = "01a3456789";
	buffer_t *    key_inserted_buff = buffer_alloc_from_bare(&key_inserted, sizeof(key_inserted));
	buffer_t *    key_delete_buff   = buffer_alloc_from_bare(&key_delete,   sizeof(key_delete));
	
	
	// create new
	hash_t  hash_create[] = {
		{ TYPE_INT32, "action", (int []){ ACTION_CRWD_CREATE }, sizeof(int) },
		{ TYPE_INT32, "size",   (int []){ 10                 }, sizeof(int) },
		hash_end
	};
	if( (ssize = backend_query(backend, hash_create, buffer)) != sizeof(off_t) )
		fail("chain in_list create failed");	
	buffer_read(buffer, 0, &key_off, MIN(ssize, sizeof(off_t)));
	
	
	// write keys
	hash_t  hash_write[] = {
		{ TYPE_INT32, "action", (int []){   ACTION_CRWD_WRITE  }, sizeof(int)   },
		{ TYPE_INT64, "key",    (off_t []){ key_off            }, sizeof(off_t) },
		{ TYPE_INT32, "size",   (int []){   10                 }, sizeof(int)   },
		hash_end
	};
	buffer_write(buffer, 0, &key_data, 10);
	ssize = backend_query(backend, hash_write, buffer);
		fail_unless(ssize == 10, "backend in_list write 1 failed");
	
	
	// insert key
	hash_t  hash_insert[] = {
		{ TYPE_INT32, "action", (int []){   ACTION_CRWD_WRITE  }, sizeof(int)   },
		{ TYPE_INT64, "key",    (off_t []){ key_off + 2        }, sizeof(off_t) },
		{ TYPE_INT32, "insert", (int []){   1                  }, sizeof(int)   },
		{ TYPE_INT32, "size",   (int []){   1                  }, sizeof(int)   },
		hash_end
	};
	buffer_write(buffer, 0, &key_insert, 1);
	ssize = backend_query(backend, hash_insert, buffer);
		fail_unless(ssize == 1,  "backend in_list write 2 failed");
	
	// check
	hash_t  hash_read[] = {
		{ TYPE_INT32, "action", (int []){   ACTION_CRWD_READ   }, sizeof(int)   },
		{ TYPE_INT64, "key",    (off_t []){ key_off            }, sizeof(off_t) },
		{ TYPE_INT32, "size",   (int []){   11                 }, sizeof(int)   },
		hash_end
	};
	buffer_cleanup(buffer);
	ssize = backend_query(backend, hash_read, buffer);
		fail_unless(ssize == 11,                                "backend in_list read 1 failed");
		fail_unless(
			buffer_memcmp(buffer, 0, key_inserted_buff, 0, ssize) == 0,
			"backend in_list read 1 data failed"
		);
	
	// delete key
	hash_t  hash_delete[] = {
		{ TYPE_INT32, "action", (int []){   ACTION_CRWD_DELETE }, sizeof(int)   },
		{ TYPE_INT64, "key",    (off_t []){ key_off + 3        }, sizeof(off_t) },
		{ TYPE_INT32, "size",   (int []){   1                  }, sizeof(int)   },
		hash_end
	};
	ssize = backend_query(backend, hash_delete, buffer);
		fail_unless(ssize == 0,  "backend in_list delete failed");
	
	// check
	hash_t  hash_read2[] = {
		{ TYPE_INT32, "action", (int []){   ACTION_CRWD_READ   }, sizeof(int)   },
		{ TYPE_INT64, "key",    (off_t []){ key_off            }, sizeof(off_t) },
		{ TYPE_INT32, "size",   (int []){   10                 }, sizeof(int)   },
		hash_end
	};
	ssize = backend_query(backend, hash_read2, buffer);
		fail_unless(ssize == 10,                                "backend in_list read 1 failed");
		fail_unless(
			buffer_memcmp(buffer, 0, key_delete_buff, 0, 10) == 0,
			"backend in_list read 1 data failed"
		);
	
	
	buffer_free(buffer);
	
	buffer_free(key_inserted_buff);
	buffer_free(key_delete_buff);
	
	backend_destroy(backend);
	setting_destroy(settings);
}
END_TEST
REGISTER_TEST(core, test_backend_list)


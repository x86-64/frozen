#include "test.h"

START_TEST (test_backend_list){
	backend_t *backend;
	
	hash_t  settings[] = {
		{ NULL, DATA_HASHT(
			{ NULL, DATA_HASHT(
				{ "name",        DATA_STRING("file")                     },
				{ "filename",    DATA_STRING("data_backend_list.dat")    },
				hash_end
			)},
			{ NULL, DATA_HASHT(
				{ "name",        DATA_STRING("list")                     },
				hash_end
			)},
			hash_end
		)},
		hash_end
	};
		
	backend = backend_new(settings);
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
		{ "action",  DATA_INT32(ACTION_CRWD_CREATE) },
		{ "size",    DATA_SIZET(10)                 },
		{ "buffer",  DATA_BUFFERT(buffer)           },
		{ "key_out", DATA_BUFFERT(buffer)           },
		hash_end
	};
	if( (ssize = backend_query(backend, hash_create)) != sizeof(off_t) )
		fail("chain in_list create failed");	
	buffer_read(buffer, 0, &key_off, sizeof(off_t));
	
	
	// write keys
	hash_t  hash_write[] = {
		{ "action", DATA_INT32(ACTION_CRWD_WRITE)  },
		{ "key",    DATA_OFFT(key_off)             },
		{ "size",   DATA_SIZET(10)                 },
		{ "buffer", DATA_BUFFERT(buffer)           },
		hash_end
	};
	buffer_write(buffer, 0, &key_data, 10);
	ssize = backend_query(backend, hash_write);
		fail_unless(ssize == 10, "backend in_list write 1 failed");
	
	// insert key
	hash_t  hash_insert[] = {
		{ "action", DATA_INT32(ACTION_CRWD_WRITE) },
		{ "key",    DATA_OFFT(key_off + 2)        },
		{ "insert", DATA_INT32(1)                 },
		{ "size",   DATA_SIZET(1)                 },
		{ "buffer", DATA_BUFFERT(buffer)         },
		hash_end
	};
	buffer_write(buffer, 0, &key_insert, 1);
	ssize = backend_query(backend, hash_insert);
		fail_unless(ssize == 1,  "backend in_list write 2 failed");
	
	// check
	hash_t  hash_read[] = {
		{ "action", DATA_INT32(ACTION_CRWD_READ)  },
		{ "key",    DATA_OFFT(key_off)            },
		{ "size",   DATA_SIZET(11)                },
		{ "buffer", DATA_BUFFERT(buffer)         },
		hash_end
	};
	buffer_cleanup(buffer);
	ssize = backend_query(backend, hash_read);
		fail_unless(ssize == 11,                                "backend in_list read 1 failed");
		fail_unless(
			buffer_memcmp(buffer, 0, key_inserted_buff, 0, ssize) == 0,
			"backend in_list read 1 data failed"
		);
	
	// delete key
	hash_t  hash_delete[] = {
		{ "action", DATA_INT32(ACTION_CRWD_DELETE)   },
		{ "key",    DATA_OFFT(key_off + 3)           },
		{ "size",   DATA_SIZET(1)                    },
		{ "buffer", DATA_BUFFERT(buffer)         },
		hash_end
	};
	ssize = backend_query(backend, hash_delete);
		fail_unless(ssize == 0,  "backend in_list delete failed");
	
	// check
	hash_t  hash_read2[] = {
		{ "action", DATA_INT32(ACTION_CRWD_READ)     },
		{ "key",    DATA_OFFT(key_off)               },
		{ "size",   DATA_SIZET(10)                   },
		{ "buffer", DATA_BUFFERT(buffer)             },
		hash_end
	};
	ssize = backend_query(backend, hash_read2);
		fail_unless(ssize == 10,                                "backend in_list read 1 failed");
		fail_unless(
			buffer_memcmp(buffer, 0, key_delete_buff, 0, 10) == 0,
			"backend in_list read 1 data failed"
		);
	
	
	buffer_free(buffer);
	
	buffer_free(key_inserted_buff);
	buffer_free(key_delete_buff);
	
	backend_destroy(backend);
}
END_TEST
REGISTER_TEST(core, test_backend_list)


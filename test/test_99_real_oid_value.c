#include <test.h>

START_TEST (test_real_oid_value){
	hash_t  config[] = {
		{ NULL, DATA_HASHT(
			{ NULL, DATA_HASHT(
				{ "name",      DATA_STRING("file")                       },
				{ "filename",  DATA_STRING("data_real_oid_value.dat")    },
				hash_end
			)},
			{ NULL, DATA_HASHT(
				{ "name",         DATA_STRING("locate")                  },
				{ "mode",         DATA_STRING("index-incapsulated")      },
				{ "oid-type",     DATA_STRING("int32")                   },
				{ "backend-type", DATA_STRING("off_t")                   },
				{ "backend",      DATA_HASHT(
					{ NULL, DATA_HASHT(
						{ "name",        DATA_STRING("file")                         },
						{ "filename",    DATA_STRING("data_real_oid_value_idx.dat")  },
						hash_end
					)},
					{ NULL, DATA_HASHT(
						{ "name",        DATA_STRING("locate")                       },
						{ "mode",        DATA_STRING("linear-incapsulated")          },
						{ "oid-type",    DATA_STRING("int32")                        },
						{ "size",        DATA_SIZET(8)                               },
						hash_end
					)},
					{ NULL, DATA_HASHT(
						{ "name",        DATA_STRING("list")                         },
						hash_end
					)},
					{ NULL, DATA_HASHT(
						{ "name",        DATA_STRING("insert-sort")                  },
						{ "sort-engine", DATA_STRING("binsearch")                    },
						{ "type",        DATA_STRING("off_t")                        },
						hash_end
					)},
					hash_end
				)},
				hash_end
			)},
			hash_end
		)},
		hash_end
	};
	int         key;
	ssize_t     ret;
	backend_t  *backend = backend_new(config);
	buffer_t   *buffer  = buffer_alloc();
	
	char  *data[] = {
		"http://google.ru/",
		"http://yandex.ru/",
		"http://bing.com/",
		"http://rambler.ru/",
		"http://aport.ru/",
		"http://hell.com/",
		"http://moooed.com/",
		NULL
	};
	char **data_ptr;
	
	// fill
	data_ptr = data;
	while(*data_ptr != NULL){
		hash_t request[] = {
			{ "action", DATA_INT32(ACTION_CRWD_WRITE)     }, // without key 'write' will call 'create' to obtain key
			{ "size",   DATA_SIZET(strlen(*data_ptr) + 1) },
			hash_end
		};
		buffer_write(buffer, 0, *data_ptr, strlen(*data_ptr) + 1);
		
		ret = backend_query(backend, request, buffer);
			fail_unless(ret > 0, "chain 'real_oid_value' write request failed");
		
		key = 0;
		buffer_read(buffer, 0, &key, MIN(ret, sizeof(int)));
		//printf("%u - %s\n", key, *data_ptr);
		
		data_ptr++;
	}
	
	// query
	char   result[21] = {0};
	hash_t req_read[] = {
		{ "action",   DATA_INT32(ACTION_CRWD_READ)   },
		{ "size",     DATA_SIZET(20)                 },
		{ "key",      DATA_OFFT(2)                   },
		hash_end
	};
	ret = backend_query(backend, req_read, buffer);
		fail_unless(ret > 0, "chain 'real_oid_value' read request failed");
	buffer_read(buffer, 0, result, MIN(ret, sizeof(result)));
		fail_unless(strcmp(result, "http://bing.com/") == 0, "chain 'real_oid_value' read data failed");
	
	// search
	/*
	char search_str[] = "http://hell.com";
	hash_t req_search[] = {
		{ "action",  DATA_INT32(ACTION_CRWD_CUSTOM)     },
		{ "size",    DATA_SIZET(strlen(search_str) + 1) },
		hash_end
	};
	buffer_write(buffer, 0, &search_str, strlen(search_str) + 1);
	ret = backend_query(backend, req_search, buffer);
		//printf("search: %x\n", ret);
	*/
	
	
	backend_destroy(backend);
	buffer_free(buffer);
}
END_TEST
REGISTER_TEST(core, test_real_oid_value)


#include <test.h>

START_TEST (test_real_oid_value){
	hash_t  config[] = {
		{ NULL, DATA_HASHT(
			{ "name",      DATA_STRING("file")                    },
			{ "filename",  DATA_STRING("data_real_oid_value.dat") },
			hash_end
		)},
		{ NULL, DATA_HASHT(
			{ "name",      DATA_STRING("locate")                  },
			{ "mode",      DATA_STRING("index-incapsulated")      },
			{ "oid-type",  DATA_STRING("int32")                   },
			{ "backend",   DATA_HASHT(
				{ NULL, DATA_HASHT(
					{ "name",        DATA_STRING("file")                         },
					{ "filename",    DATA_STRING("data_real_oid_value_idx.dat")  },
					hash_end
				)},
				{ NULL, DATA_HASHT(
					{ "name",        DATA_STRING("locate")                       },
					{ "mode",        DATA_STRING("linear-incapsulated")          },
					{ "oid-type",    DATA_STRING("int32")                        },
					{ "size",        DATA_INT32(4)                               },
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
	};
	ssize_t     ret;
	backend_t  *backend = backend_new("real_oid_value", config);
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
	
	data_ptr = data;
	while(*data_ptr != NULL){
		hash_t request[] = {
			{ "action", DATA_INT32(ACTION_CRWD_CREATE)  }, // without key 'write' will call 'create' to obtain key
			{ "size",   DATA_INT32(strlen(*data_ptr))   },
			hash_end
		};
		buffer_write(buffer, 0, *data_ptr, strlen(*data_ptr) + 1);
		
		ret = backend_query(backend, request, buffer);
			fail_unless(ret > 0, "chain 'real_oid_value' request failed");
		
		data_ptr++;
	}
	
	backend_destroy(backend);
	buffer_free(buffer);
}
END_TEST
REGISTER_TEST(core, test_real_oid_value)


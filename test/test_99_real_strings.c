#include <test.h>

START_TEST (test_real_store_strings){
	hash_t config[] = {
		{ "chains", DATA_HASHT(
			{ NULL, DATA_HASHT(
				{ "name",      DATA_STRING("file")                        },
				{ "filename",  DATA_STRING("data_real_store_strings.dat") },
				hash_end
			)},
			hash_end
		)},
		hash_end
	};
	
	backend_t  *backend = backend_new(config);
	
	off_t  data_ptrs[6];
	char  *data_array[] = {
		"http://google.ru/",
		"http://yandex.ru/",
		"http://bing.com/",
		"http://rambler.ru/",
		"http://aport.ru/",
		"http://hell.com/"
	};
	
	// write array to file
	int      i;
	ssize_t  ret;
	
	for(i=0; i < sizeof(data_array) / sizeof(char *); i++){
		request_t r_write[] = {
			{ "action",  DATA_INT32 (ACTION_CRWD_WRITE)                           },
			{ "key_out", DATA_PTR_OFFT   (&data_ptrs[i])                          },
			{ "buffer",  DATA_PTR_STRING (data_array[i], strlen(data_array[i])+1) },
			hash_end
		};
		ret = backend_query(backend, r_write);
			fail_unless(ret > 0, "chain 'real_store_strings': write array failed");
	}
	
	// check
	char data_read[1024];
	for(i=0; i < sizeof(data_array) / sizeof(char *); i++){
		request_t r_read[] = {
			{ "action", DATA_INT32(ACTION_CRWD_READ)      },
			{ "key",    DATA_OFFT(data_ptrs[i])           },
			{ "buffer", DATA_PTR_STRING(&data_read, 1024) },
			hash_end
		};
		ret = backend_query(backend, r_read);
			fail_unless(ret > 0,                               "chain 'real_store_strings': read array failed");
			fail_unless(strcmp(data_read, data_array[i]) == 0, "chain 'real_store_strings': read array data failed");
	}
	
	backend_destroy(backend);
}
END_TEST
REGISTER_TEST(core, test_real_store_strings)




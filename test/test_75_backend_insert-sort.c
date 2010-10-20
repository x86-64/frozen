#include <test.h>
#include <sys/time.h>

START_TEST (test_backend_insert_sort){
	backend_t      *backend;
	
	hash_set(global_settings, "homedir", DATA_STRING("."));
	
	hash_t  settings[] = {
		{ NULL, DATA_HASHT(
			{ "name",        DATA_STRING("file")                     },
			{ "filename",    DATA_STRING("data_backend_insert_sort") },
			hash_end
		)},
		{ NULL, DATA_HASHT(
			{ "name",        DATA_STRING("list")                     },
			hash_end
		)},
		{ NULL, DATA_HASHT(
			{ "name",        DATA_STRING("insert-sort")              },
			{ "sort-engine", DATA_STRING("binsearch")                },
			{ "type",        DATA_STRING("int8")                     },
			hash_end
		)},
		hash_end
	};
	
	if( (backend = backend_new("in_sort", settings)) == NULL){
		fail("backend creation failed");
		return;
	}
	
	char           buff;
	buffer_t       temp;
	buffer_t       buffer;
	ssize_t        ret;
	int            i, iters = 10;
	struct timeval tv;
	
	buffer_init(&temp);
	buffer_init_from_bare(&buffer, &buff, sizeof(buff));
	
	for(i=0; i<iters; i++){
		gettimeofday(&tv, NULL);
		
		buff = (char)(0x61 + (tv.tv_usec % 26));
		hash_t  req_write[] = {
			{ "action", DATA_INT32(ACTION_CRWD_WRITE) },
			{ "key",    DATA_OFFT(0)                  },
			{ "size",   DATA_INT32(sizeof(buff))      },
			hash_end
		};
		ret = backend_query(backend, req_write, &buffer);
			fail_unless(ret == sizeof(buff),  "chain in_sort write failed");
	}
	
	hash_t  req_read[] = {
		{ "action", DATA_INT32(ACTION_CRWD_READ) },
		{ "key",    DATA_OFFT(0)                 },
		{ "size",   DATA_INT32(iters)            },
		hash_end
	};
	ret = backend_query(backend, req_read, &temp);
		fail_unless(ret == iters, "chain in_sort read failed");
	
	for(i=0; i<iters-1; i++){
		if(buffer_memcmp(&temp, i, &temp, i+1, 1) > 0)
			fail("chain in_sort wrong inserts");
	}
	
	buffer_destroy(&buffer);
	buffer_destroy(&temp);
	
	backend_destroy(backend);
}
END_TEST
REGISTER_TEST(core, test_backend_insert_sort)


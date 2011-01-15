#include <test.h>
#include <sys/time.h>

START_TEST (test_backend_insert_sort){
	backend_t      *backend;
	
	hash_t  settings[] = {
		{ HK(chains), DATA_HASHT(
			{ 0, DATA_HASHT(
				{ HK(name),        DATA_STRING("file")                         },
				{ HK(filename),    DATA_STRING("data_backend_insert_sort.dat") },
				hash_end
			)},
			{ 0, DATA_HASHT(
				{ HK(name),        DATA_STRING("list")                     },
				hash_end
			)},
			{ 0, DATA_HASHT(
				{ HK(name),        DATA_STRING("insert-sort")              },
				{ HK(engine),      DATA_STRING("binsearch")                },
				hash_end
			)},
			hash_end
		)},
		hash_end
	};
	
	if( (backend = backend_new(settings)) == NULL){
		fail("backend creation failed");
		return;
	}
	
	char           buff;
	char           temp[256];
	off_t          key;
	ssize_t        ret;
	int            i, iters = 10;
	struct timeval tv;
	
	for(i=0; i<iters; i++){
		gettimeofday(&tv, NULL);
		
		buff = (char)(0x61 + (tv.tv_usec % 26));
		hash_t  req_write[] = {
			{ HK(action),     DATA_INT32(ACTION_CRWD_WRITE) },
			{ HK(offset_out), DATA_PTR_OFFT(&key)           },
			{ HK(buffer),     DATA_PTR_INT8(&buff)          },
			hash_end
		};
		ret = backend_query(backend, req_write);
			fail_unless(ret > 0,  "chain in_sort write failed");
	}
	
	hash_t  req_read[] = {
		{ HK(action), DATA_INT32(ACTION_CRWD_READ) },
		{ HK(offset), DATA_OFFT(0)                 },
		{ HK(size),   DATA_SIZET(iters)            },
		{ HK(buffer), DATA_MEM(&temp, 256)         },
		hash_end
	};
	ret = backend_query(backend, req_read);
		fail_unless(ret == iters, "chain in_sort read failed");
	
	for(i=0; i<iters-1; i++){
		if(temp[i] > temp[i+1])
			fail("chain in_sort wrong inserts");
	}
	
	backend_destroy(backend);
}
END_TEST
REGISTER_TEST(core, test_backend_insert_sort)


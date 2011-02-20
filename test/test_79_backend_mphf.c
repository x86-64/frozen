
/*
START_TEST (test_backend_mphf){
	hash_t config[] = {
		{ 0, DATA_HASHT(
			{ HK(name),    DATA_STRING("backend_mphf_idx")                        },
			{ HK(chains),  DATA_HASHT(
				{ 0, DATA_HASHT(
					{ HK(name),      DATA_STRING("file")                  },
					{ HK(filename),  DATA_STRING("data_backend_mphi.dat") },
					hash_end
				)},
				hash_end
			)},
			hash_end
		)},
		{ 0, DATA_HASHT(
			{ HK(name),   DATA_STRING("backend_mphf")                             },
			{ HK(chains), DATA_HASHT(
				{ 0, DATA_HASHT(
					{ HK(name),      DATA_STRING("file")                  },
					{ HK(filename),  DATA_STRING("data_backend_mphf.dat") },
					hash_end
				)},
				{ 0, DATA_HASHT(
					{ HK(name),       DATA_STRING("mphf")                 },
					{ HK(mphf_type),  DATA_STRING("chm_imp")              },
					{ HK(backend),    DATA_STRING("backend_mphf_idx")     },
					{ HK(value_bits), DATA_INT32(31)                      },
					hash_end
				)},
				hash_end
			)},
			hash_end
		)},
		hash_end
	};
	
	backend_bulk_new(config);
	
	data_t     n_dat = DATA_STRING("backend_mphf"), n_idx = DATA_STRING("backend_mphf_idx");
	backend_t *b_dat = backend_find(&n_dat);
	backend_t *b_idx = backend_find(&n_idx);
	
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
			{ HK(action),  DATA_INT32 (ACTION_CRWD_WRITE)                           },
			{ HK(key),     DATA_PTR_STRING (data_array[i], strlen(data_array[i])+1) },
			{ HK(buffer),  DATA_PTR_STRING (data_array[i], strlen(data_array[i])+1) },
			hash_end
		};
		ret = backend_query(b_dat, r_write);
			fail_unless(ret >= 0, "chain 'backend_mphf': write array failed");
	}
	
	// check
	char data_read[1024];
	for(i=0; i < sizeof(data_array) / sizeof(char *); i++){
		memset(data_read, 0, 1024);
		
		request_t r_read[] = {
			{ HK(action), DATA_INT32(ACTION_CRWD_READ)                              },
			{ HK(key),    DATA_PTR_STRING (data_array[i], strlen(data_array[i])+1)  },
			{ HK(buffer), DATA_PTR_STRING (&data_read, 1024)                        },
			hash_end
		};
		ret = backend_query(b_dat, r_read);
			fail_unless(ret >= 0,                              "chain 'backend_mphf': read array failed");
			fail_unless(strcmp(data_read, data_array[i]) == 0, "chain 'backend_mphf': read array data failed");
	}
	
	backend_destroy(b_dat);
	backend_destroy(b_idx);
}
END_TEST
REEEGISTER_TEST(core, test_backend_mphf)
*/

/* benchmarks {{{ */
typedef struct bench_t {
	struct timeval  tv_start;
	struct timeval  tv_end;
	struct timeval  tv_diff;
	unsigned long   time_ms;
	unsigned long   time_us;
	char            string[255];
} bench_t;

void timeval_subtract (result, x, y)
        struct timeval *result, *x, *y; 
{
    
        if (x->tv_usec < y->tv_usec) {
                int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
                y->tv_usec -= 1000000 * nsec;
                y->tv_sec += nsec;
        }   
        if (x->tv_usec - y->tv_usec > 1000000) {
                int nsec = (x->tv_usec - y->tv_usec) / 1000000;
                y->tv_usec += 1000000 * nsec;
                y->tv_sec -= nsec;
        }   
        result->tv_sec = x->tv_sec - y->tv_sec;
        result->tv_usec = x->tv_usec - y->tv_usec;
}


bench_t * bench_start(void){
        bench_t *bench = calloc(sizeof(bench_t), 1); 
        gettimeofday(&bench->tv_start, NULL);
        return bench; 
}
void bench_free(bench_t *bench){
	free(bench);
}
void bench_query(bench_t *bench){
	gettimeofday(&bench->tv_end, NULL);
    
        timeval_subtract(&bench->tv_diff, &bench->tv_end, &bench->tv_start);
        sprintf(bench->string, "%3u.%.6u", (unsigned int)bench->tv_diff.tv_sec, (unsigned int)bench->tv_diff.tv_usec);
	
	bench->time_ms = bench->tv_diff.tv_usec / 1000  + bench->tv_diff.tv_sec * 1000;
	bench->time_us = bench->tv_diff.tv_usec         + bench->tv_diff.tv_sec * 1000000;
	
	if(bench->time_ms == 0) bench->time_ms = 1;
	if(bench->time_us == 0) bench->time_us = 1;
}
/* }}} */

#define NTESTS 100

START_TEST (test_backend_mphf_speed){
	hash_t config[] = {
		{ 0, DATA_HASHT(
			{ HK(name),    DATA_STRING("backend_mphf_idx")                        },
			{ HK(chains),  DATA_HASHT(
				{ 0, DATA_HASHT(
					{ HK(name),      DATA_STRING("file")                  },
					{ HK(filename),  DATA_STRING("data_backend_mphis.dat") },
					hash_end
				)},
				{ 0, DATA_HASHT(
					{ HK(name),       DATA_STRING("cache")                 },
					hash_end
				)},
				hash_end
			)},
			hash_end
		)},
		{ 0, DATA_HASHT(
			{ HK(name),   DATA_STRING("backend_mphf")                              },
			{ HK(chains), DATA_HASHT(
				{ 0, DATA_HASHT(
					{ HK(name),       DATA_STRING("file")                  },
					{ HK(filename),   DATA_STRING("data_backend_mphfs.dat")},
					hash_end
				)},
				{ 0, DATA_HASHT(
					{ HK(name),       DATA_STRING("cache")                 },
					hash_end
				)},
				{ 0, DATA_HASHT(
					{ HK(name),       DATA_STRING("incapsulate")           },
					{ HK(multiply),   DATA_OFFT(8)                         },
					hash_end
				)},
				{ 0, DATA_HASHT(
					{ HK(name),       DATA_STRING("structs")               },
					{ HK(size),       DATA_STRING("size")                  },
					{ HK(structure),  DATA_HASHT(
						{ HK(key), DATA_STRING("")                     },
						hash_end
					)},
					hash_end
				)},
				{ 0, DATA_HASHT(
					{ HK(name),        DATA_STRING("mphf")                 },
					{ HK(type),        DATA_STRING("chm_imp")              },
					{ HK(backend),     DATA_STRING("backend_mphf_idx")     },
					{ HK(nelements),   DATA_INT64(NTESTS)                  },
					{ HK(value_bits),  DATA_INT32(31)                      },
					{ HK(persistent),  DATA_INT32(1)                       },
					{ HK(build_start), DATA_STRING("onload")               },
					{ HK(build_end),   DATA_STRING("onunload")             },
					hash_end
				)},
				hash_end
			)},
			hash_end
		)},
		hash_end
	};
	
	backend_bulk_new(config);
	
	data_t     n_dat = DATA_STRING("backend_mphf"), n_idx = DATA_STRING("backend_mphf_idx");
	backend_t *b_dat = backend_find(&n_dat);
	backend_t *b_idx = backend_find(&n_idx);
		fail_unless(b_dat != NULL && b_idx != NULL, "chain 'backend_mphf' backend creating failed");

	// write array to file
	int      i,j;
	ssize_t  failed;
	size_t   ntests = NTESTS;
	char     test[8], test2[8];
	
	request_t r_write[] = {
		{ HK(action),  DATA_INT32(ACTION_CRWD_CREATE) },
		{ HK(buffer),  DATA_RAW(test2, sizeof(test2)) },
		
		{ HK(key),     DATA_PTR_STRING(test, 8)       },
		hash_end
	};
	
	memset(test, 'a', sizeof(test));
	test[7] = '\0';
	failed = 0;
	
	bench_t  *be = bench_start();
	for(i=0; i < ntests; i++){
		// inc test
		for(j=0;j<sizeof(test)-1;j++){
			if(++test[j] != 'z') break;
			test[j] = 'a';
		}
		
		if(backend_query(b_dat, r_write) < 0)
			failed++;
	}
	bench_query(be);
	printf("mphf write: %u iters took %s sec; speed: %7lu iters/s (one: %10lu ns) failed: %d\n",
		ntests,
		be->string,
		( ntests * 1000 / be->time_ms), 
		( be->time_us * 1000 / ntests ),
		(int)failed
	);
	
	bench_free(be);
	fail_unless(failed == 0, "chain 'backend_mphf': write array failed");
	
	// read test
	memset(test, 'a', sizeof(test));
	test[7] = '\0';
	failed = 0;
	
	be = bench_start();
	for(i=0; i < ntests; i++){
		// inc test
		for(j=0;j<sizeof(test)-1;j++){
			if(++test[j] != 'z') break;
			test[j] = 'a';
		}
		
		request_t r_read[] = {
			{ HK(action),  DATA_INT32(ACTION_CRWD_READ)   },
			{ HK(key),     DATA_RAW(test,  sizeof(test))  },
			{ HK(buffer),  DATA_RAW(test2, sizeof(test2)) },
			{ HK(size),    DATA_SIZET(sizeof(test2))      },
			hash_end
		};
		
		if(backend_query(b_dat, r_read) < 0 || memcmp(test, test2, sizeof(test)) != 0){
			failed++;
			//printf("failed: %.*s %.*s\n", sizeof(test), test, sizeof(test2), test2);
		}
	}
	bench_query(be);
	printf("mphf  read: %u iters took %s sec; speed: %7lu iters/s (one: %10lu ns) failed: %d\n",
		ntests,
		be->string,
		( ntests * 1000 / be->time_ms), 
		( be->time_us / ntests * 1000 ),
		(int)failed
	);
	
	bench_free(be);
	fail_unless(failed == 0, "chain 'backend_mphf': read array failed");
	
	backend_destroy(b_dat);
	backend_destroy(b_idx);
}
END_TEST
REGISTER_TEST(core, test_backend_mphf_speed)


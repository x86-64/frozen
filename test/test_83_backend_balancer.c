START_TEST (test_backend_balancer_counting){
	backend_t *backend;
	
	hash_t  settings[] = {
                { 0, DATA_HASHT(
                        { HK(class),       DATA_STRING("file")                         },
                        { HK(filename),    DATA_HASHT(
				{ HK(homedir), DATA_VOID                               },
				{ HK(string),  DATA_STRING("data_backend_bal_count_")  },
				{ HK(random),  DATA_STRING("AAAA")                     },
				{ HK(string),  DATA_STRING(".dat")                     },
				hash_end
			)},
                        { HK(fork_only),   DATA_SIZET(1)                               },
			hash_end
                )},
                { 0, DATA_HASHT(
                        { HK(class),       DATA_STRING("balancer")                     },
                        { HK(mode),        DATA_STRING("counting")                     },
                        { HK(pool),        DATA_STRING("fork")                         },
                        { HK(pool_size),   DATA_SIZET(5)                               },
                        hash_end
                )},
		
                hash_end
	};
	
	backend = backend_new(settings);
		fail_unless(backend != NULL, "backend creation failed");
	
	size_t i, ret, size;
	
	for(i=0; i<10; i++){
		request_t r_create[] = {
			{ HK(action), DATA_UINT32T(ACTION_CREATE) },
			{ HK(size),   DATA_SIZET(10)                   },
			{ HK(ret),    DATA_PTR_SIZET(&ret)             },
			hash_end
		};
		backend_query(backend, r_create);
			fail_unless(ret >= 0, "r_create failed");
	};
	for(i=0; i<5; i++){
		request_t r_count[] = {
			{ HK(action), DATA_UINT32T(ACTION_COUNT)  },
			{ HK(buffer), DATA_PTR_SIZET(&size)            },
			hash_end
		};
		backend_query(backend, r_count);
			fail_unless(size == 20, "count failed");
	}
	
	backend_destroy(backend);
}
END_TEST
REGISTER_TEST(core, test_backend_balancer_counting)

START_TEST (test_backend_balancer_linear){
	backend_t *backend;
	
	hash_t  settings[] = {
                { 0, DATA_HASHT(
                        { HK(class),       DATA_STRING("file")                         },
                        { HK(filename),    DATA_HASHT(
				{ HK(homedir), DATA_VOID                               },
				{ HK(string),  DATA_STRING("data_backend_bal_linear_") },
				{ HK(fork),    DATA_STRING("buffer")                   },
				{ HK(string),  DATA_STRING(".dat")                     },
				hash_end
			)},
                        { HK(fork_only),   DATA_SIZET(1)                               },
			hash_end
                )},
                { 0, DATA_HASHT(
                        { HK(class),       DATA_STRING("balancer")                     },
                        { HK(mode),        DATA_STRING("linear")                       },
                        { HK(field),       DATA_STRING("buffer")                       },
			{ HK(linear_len),  DATA_SIZET(1)                               },
			hash_end
                )},
		
                hash_end
	};
	
	backend = backend_new(settings);
		fail_unless(backend != NULL, "backend creation failed");
	
	size_t i, ret, size;
	char   buf[10] = {0};
	
	for(i=0; i<20; i++){
		buf[0] = 'a' + (i % 10);
		
		request_t r_create[] = {
			{ HK(action), DATA_UINT32T(ACTION_CREATE) },
			{ HK(buffer), DATA_PTR_STRING(buf)             },
			{ HK(size),   DATA_SIZET(10)                   },
			{ HK(ret),    DATA_PTR_SIZET(&ret)             },
			hash_end
		};
		backend_query(backend, r_create);
			fail_unless(ret >= 0, "r_create failed");
	};
	for(i=0; i<10; i++){
		*((char *)&size) = 'a' + i;
		
		request_t r_count[] = {
			{ HK(action), DATA_UINT32T(ACTION_COUNT)  },
			{ HK(buffer), DATA_PTR_SIZET(&size)            },
			hash_end
		};
		backend_query(backend, r_count);
			fail_unless(size == 20, "count failed");
	}
	
	backend_destroy(backend);
}
END_TEST
REGISTER_TEST(core, test_backend_balancer_linear)


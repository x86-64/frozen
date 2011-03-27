
START_TEST (test_backend_cache_app){
	backend_t *backend;
	
	hash_t  settings[] = {
		{ HK(backends), DATA_HASHT(
			{ 0, DATA_HASHT(
				{ HK(name),        DATA_STRING("file")                          },
				{ HK(filename),    DATA_STRING("data_backend_cache_app.dat")    },
				hash_end
			)},
			{ 0, DATA_HASHT(
				{ HK(name),        DATA_STRING("cache-append")                  },
                                { HK(size),        DATA_SIZET(100)                              },
				hash_end
			)},
			hash_end
		)},
		hash_end
	};
	
	/* create backend */
	backend = backend_new(settings);
		fail_unless(backend != NULL, "backend creation failed");
	
	/* test read/writes */
	ssize_t         ret;
	size_t          i;
        char            buf[30] = {0};
        

        for(i=0; i<=10; i++){
                request_t       request[] = {
                        { HK(action),  DATA_UINT32T(ACTION_CRWD_CREATE)  },
                        { HK(buffer),  DATA_RAW(buf, 30)               },
                        { HK(size),    DATA_SIZET(30)                  },
                        hash_end
                };
                
                ret = backend_query(backend, request);
                        fail_unless(ret >= 0, "write failed");
        }
	
	backend_destroy(backend);
}
END_TEST
REGISTER_TEST(core, test_backend_cache_app)




START_TEST (test_backends_two_backends){
	backend_t *backend;
	
	hash_t  settings[] = {
                { 0, DATA_HASHT(
                        { HK(class),        DATA_STRING("file")                     },
                        { HK(filename),     DATA_STRING("data_backend_file.dat")    },
                        hash_end
                )},
                { 0, DATA_HASHT(
                        { HK(class),        DATA_STRING("null")                     },
                        hash_end
                )},
                hash_end
	};
	
	/* create backend */
	backend = backend_new(settings);
		fail_unless(backend != NULL, "backend creation failed");
	
	/* test read/writes */
	buffer_t       *buffer  = buffer_alloc();
	ssize_t         ssize;
	hash_t          request[] = {
		{ HK(action),  DATA_UINT32T(ACTION_CRWD_CREATE)  },
		{ HK(size),    DATA_SIZET( 0xBEEF )            },
		{ HK(buffer),  DATA_BUFFERT(buffer)            },
		hash_end
	};
	
	ssize = backend_query(backend, request);
		fail_unless(ssize == 0x0000BEEF, "backend backend incorrectly set");
		
	buffer_free(buffer);
	
	// rest will work, won't test
	
	backend_destroy(backend);
}
END_TEST
REGISTER_TEST(core, test_backends_two_backends)


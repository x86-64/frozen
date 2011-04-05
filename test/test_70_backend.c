
START_TEST (test_backends){
	backend_t *backend, *backendf;
	
	hash_t  settings[] = {
                { 0, DATA_HASHT(
                        { HK(class),       DATA_STRING("file")                     },
                        { HK(filename),    DATA_STRING("data_backend_file.dat")    },
                        hash_end
                )},
                hash_end
	};
	
	/* re-run with good parameters */
	backend = backend_new(settings);
		fail_unless(backend != NULL, "backend creation failed");
	
	request_t r_fork[] = {
		hash_end
	};
	
	backendf = backend_fork(backend, r_fork);
		fail_unless(backendf != NULL, "backend fork creation failed");
	
	backend_destroy(backendf);
	backend_destroy(backend);
}
END_TEST
REGISTER_TEST(core, test_backends)


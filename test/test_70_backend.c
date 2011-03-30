
START_TEST (test_backends){
	backend_t *backend;
	
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
	
	backend_destroy(backend);
}
END_TEST
REGISTER_TEST(core, test_backends)


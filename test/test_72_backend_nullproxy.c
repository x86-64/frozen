#include <test.h>

START_TEST (test_backends_two_chains){
	backend_t *backend;
	
	hash_t  settings[] = {
		{ NULL, DATA_HASHT(
			{ NULL, DATA_HASHT(
				{ "name",        DATA_STRING("file")                     },
				{ "filename",    DATA_STRING("data_backend_file.dat")    },
				hash_end
			)},
			{ NULL, DATA_HASHT(
				{ "name",        DATA_STRING("null-proxy")               },
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
	buffer_t       *buffer  = buffer_alloc();
	ssize_t         ssize;
	hash_t          request[] = {
		{ "action",  DATA_INT32(ACTION_CRWD_CREATE)  },
		{ "size",    DATA_SIZET( 0xBEEF )            },
		hash_end
	};
	
	ssize = backend_query(backend, request, buffer);
		fail_unless(ssize == 0x0000BEEF, "backend chain incorrectly set");
		
	buffer_free(buffer);
	
	// rest will work, won't test
	
	backend_destroy(backend);
}
END_TEST
REGISTER_TEST(core, test_backends_two_chains)


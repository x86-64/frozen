#include "test.h"

START_TEST (test_backends){
	backend_t *backend;
	
	hash_t  settings[] = {
		{ NULL, DATA_HASHT(
			{ NULL, DATA_HASHT(
				{ "name",        DATA_STRING("file")                     },
				{ "filename",    DATA_STRING("data_backend_file.dat")    },
				hash_end
			)},
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


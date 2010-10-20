#include "test.h"

START_TEST (test_backends){
	backend_t *backend, *found;
	
	hash_set(global_settings, "homedir", DATA_STRING("."));
	
	hash_t  settings[] = {
		{ NULL, DATA_HASHT(
			{ "name",        DATA_STRING("file")                     },
			{ "filename",    DATA_STRING("data_backend_file")        },
			hash_end
		)},
		hash_end
	};
	
	/* re-run with good parameters */
	backend = backend_new("in_file", settings);
		fail_unless(backend != NULL, "backend creation failed");
	
	found = backend_find_by_name("in_file");
		fail_unless(backend == found, "backend_find_by_name not working");
	
	backend_destroy(backend);
}
END_TEST
REGISTER_TEST(core, test_backends)


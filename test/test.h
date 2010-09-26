#include "libfrozen.h"
#include <stdlib.h>
#include <check.h>

extern TCase *tc_core;

#define REGISTER_TEST(suite, test_func) \
	static void __attribute__((constructor))__test_init_##test_func() { \
		tcase_add_test(tc_##suite, test_func);                      \
        }



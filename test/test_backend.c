#include "../src/libfrozen.h"
#include "../src/backend.h"
#include "check_main.h"

int chains_registred = 0;
void * chain_callb(void *arg, void *null1, void *null2){
	chains_registred++;
}

START_TEST (test_chains){
	list_iter(&chains, &chain_callb, NULL, NULL);
	fail_unless( chains_registred > 0, "No chains registered");
}
END_TEST
REGISTER_TEST(core, test_chains)


#include "test.h"
#include "../src/backend.h"
#include "../src/settings.h"

START_TEST (test_backend_file){
	int ret;
	
	chain_t *chain = chain_new("file");
		fail_unless(chain != NULL, "chain 'file' not created");
	
	setting_t *setting = setting_new();
	setting_set_child_string(global_settings, "homedir",  ".");
	setting_set_child_string(setting,         "filename", "data_backend_file");
	
	ret = chain_configure(chain, setting);
		fail_unless(ret == 0, "chain 'file' configuration failed");
	
	off_t new_key1, new_key2;
	
	ret = chain_crwd_create(chain, &new_key1, 10);
		fail_unless(ret == 0, "chain 'file' create key1 failed");
	ret = chain_crwd_create(chain, &new_key2, 10);
		fail_unless(ret == 0, "chain 'file' create key2 failed");
		fail_unless(new_key2 - new_key1 == 10, "chain 'file' offsets invalid");
	
	char *test1_val = strdup("test167890");
	char *test2_val = strdup("test267890");
	char test_buff[256];
	
	ret = chain_crwd_set(chain, &new_key1, test1_val, 10);
		fail_unless(ret == 0, "chain 'file' set key1 failed");
	ret = chain_crwd_set(chain, &new_key2, test2_val, 10);
		fail_unless(ret == 0, "chain 'file' set key2 failed");
	
	ret = chain_crwd_get(chain, &new_key1, &test_buff, 10);
		fail_unless(ret == 0 && strncmp(test_buff, test1_val, 10) == 0, "chain 'file' get key1 failed");
	ret = chain_crwd_get(chain, &new_key2, &test_buff, 10);
		fail_unless(ret == 0 && strncmp(test_buff, test2_val, 10) == 0, "chain 'file' get key2 failed");
	
	ret = chain_crwd_delete(chain, &new_key1, 10);
		fail_unless(ret == 0, "chain 'file' delete key1 failed");
	ret = chain_crwd_delete(chain, &new_key2, 10);
		fail_unless(ret == 0, "chain 'file' delete key2 failed");
	
	free(test1_val);
	free(test2_val);
	
	setting_destroy(setting);
	chain_destroy(chain);
	
}
END_TEST
REGISTER_TEST(core, test_backend_file)



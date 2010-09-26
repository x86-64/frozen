#include "test.h"

START_TEST (test_settings_basic){
	setting_t *setting = setting_new();
		fail_unless( setting != NULL,                "Setting not created");
		fail_unless( setting->type == SETTING_NONE,  "Setting default type not null"); 
	
	setting_empty(setting);
		fail_unless( setting->type == SETTING_NONE,  "Setting after empty is not valid");
	
	setting_destroy(setting);
		fail_unless( 1 == 1, "Setting destory not work");
}
END_TEST
REGISTER_TEST(core, test_settings_basic)

START_TEST (test_settings_strings){
	char      *test;
	setting_t *setting = setting_new();
	
	setting_set_string(setting, "hello");
		fail_unless(setting->type == SETTING_STRING,              "set_string Setting type invalid");
		fail_unless(strcmp(setting->string.value, "hello") == 0,  "set_string Setting value invalid (not hello)");
	test = setting_get_string(setting);
		fail_unless(strcmp(test, "hello") == 0,                   "set_string Setting get_sting invalid");
	
	setting_set_string(setting, NULL);
		fail_unless(setting->type == SETTING_STRING,              "set_string(null) type invalid");
		fail_unless(setting->string.value == NULL,                "set_string(null) value invalid");
	test = setting_get_string(setting);
		fail_unless(test == NULL,                                 "get_string(null) invalid");
	
	setting_destroy(setting);
}
END_TEST
REGISTER_TEST(core, test_settings_strings)


int test_child_iter(void *p_setting, void *p_count, void *null){
	*((int *)p_count) = *((int *)p_count) + 1;
	
	return ITER_CONTINUE;
}

START_TEST (test_settings_childs){
	char      *test;
	setting_t *setting = setting_new();
	
	/* simple set\get */
	setting_set_child_string(setting, "test1", "test1_value");
		fail_unless(setting->type == SETTING_LIST,                     "set_child_string type invalid");
	test = setting_get_child_string(setting, "test1");
		fail_unless(test != NULL && strcmp(test, "test1_value") == 0,  "get_child_string invalid ret value");
	
	/* two settings in list */
	setting_set_child_string(setting, "test2", "test2_value");
	
	test = setting_get_child_string(setting, "test1");
		fail_unless(test != NULL && strcmp(test, "test1_value") == 0,  "get_child_string test1 invalid");
	test = setting_get_child_string(setting, "test2");
		fail_unless(test != NULL && strcmp(test, "test2_value") == 0,  "get_child_string test2 invalid");
	
	/* set\get child setting_t */
	setting_t *new_setting = setting_new_named("test2");
	setting_set_string(new_setting, "test2_value_new");
	
	setting_set_child_setting(setting, new_setting);
	
	test = setting_get_child_string(setting, "test2");
		fail_unless(test != NULL && strcmp(test, "test2_value_new") == 0,  "get_child_string test2_new invalid");
	
	setting_t *fetch_setting = setting_get_child_setting(setting, "test2");
		fail_unless(fetch_setting == new_setting,                          "get_child_setting failed");
	
	/* destroying childs */
	setting_destroy_child(setting, "test2");
	
	test = setting_get_child_string(setting, "test2");
		fail_unless(test == NULL,                                          "destroy_child not work");
	
	/* iterating childs */
	
	int childs = 0;
	setting_iter_child(setting, (iter_callback)test_child_iter, &childs, 0);
		fail_unless(childs == 1,                                           "iter_childs not work");

	setting_destroy(setting);
}
END_TEST
REGISTER_TEST(core, test_settings_childs)


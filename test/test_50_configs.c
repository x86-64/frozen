
START_TEST (test_configs){
	hash_t *hash, *key1, *key2, *key3, *key4, *subhash;
	
	char   test1[] = "key1 = 'hello1', key2 : \"hello2\", key3 => 'hello3'";
	hash = configs_string_parse(test1);
		fail_unless(hash != NULL,  "configs test1 failed");
	
	fail_unless( (key1 = hash_find(hash, "key1")) != NULL, "hash not found existing entry");
	fail_unless( (key2 = hash_find(hash, "key2")) != NULL, "hash not found existing entry");
	fail_unless( (key3 = hash_find(hash, "key3")) != NULL, "hash not found existing entry");
	fail_unless( (hash_find(hash, "key999")) == NULL,      "hash found not existing entry");
	
	fail_unless(strcmp(hash_get_value_ptr(key1), "hello1") == 0, "hash found wrong entry");
	fail_unless(strcmp(hash_get_value_ptr(key2), "hello2") == 0, "hash found wrong entry");
	fail_unless(strcmp(hash_get_value_ptr(key3), "hello3") == 0, "hash found wrong entry");
	
	hash_free(hash, 1);
	
	char   test2[] = "key1 => { key3 => 'hello1', key4 => \"hello2\"}, key2 => 'hello3', key5 => {}";
	hash = configs_string_parse(test2);
		fail_unless(hash != NULL,  "configs test1 failed");
	
	fail_unless( (key1 = hash_find(hash, "key1")) != NULL, "hash not found existing entry");
	fail_unless( (key2 = hash_find(hash, "key2")) != NULL, "hash not found existing entry");
	
	subhash = (hash_t *)hash_get_value_ptr(key1);
	fail_unless( (key3 = hash_find(subhash, "key3")) != NULL, "hash not found existing entry");
	fail_unless( (key4 = hash_find(subhash, "key4")) != NULL, "hash not found existing entry");
	fail_unless( (hash_find(subhash, "key999"))      == NULL, "hash found not existing entry");
	
	fail_unless(strcmp(hash_get_value_ptr(key2), "hello3") == 0, "hash found wrong entry");
	fail_unless(strcmp(hash_get_value_ptr(key3), "hello1") == 0, "hash found wrong entry");
	fail_unless(strcmp(hash_get_value_ptr(key4), "hello2") == 0, "hash found wrong entry");
	
	hash_free(hash, 1);
}
END_TEST
REGISTER_TEST(core, test_configs)

